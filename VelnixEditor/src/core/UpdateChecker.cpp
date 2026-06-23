#include "core/UpdateChecker.h"

#include <curl/curl.h>
#include <gio/gio.h>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#ifndef VELNIX_UPDATE_URL
#define VELNIX_UPDATE_URL "https://bajielyf.github.io/velnix-editor/version.txt"
#endif

namespace {

struct AsyncRequest {
  UpdateChecker::Callback callback;
};

std::once_flag curlInitFlag;

size_t append_response(char *data, size_t size, size_t count, void *output) {
  const size_t bytes = size * count;
  static_cast<std::string *>(output)->append(data, bytes);
  return bytes;
}

std::string trim(std::string value) {
  const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), isNotSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(),
              value.end());
  return value;
}

std::vector<int> parse_version(const std::string &rawVersion) {
  std::string version = trim(rawVersion);
  if (!version.empty() && (version.front() == 'v' || version.front() == 'V')) {
    version.erase(version.begin());
  }

  std::vector<int> parts;
  std::stringstream stream(version);
  std::string part;
  while (std::getline(stream, part, '.')) {
    if (part.empty() ||
        !std::all_of(part.begin(), part.end(),
                     [](unsigned char ch) { return std::isdigit(ch); })) {
      return {};
    }
    try {
      parts.push_back(std::stoi(part));
    } catch (...) {
      return {};
    }
  }
  return parts;
}

void run_check(GTask *task, gpointer, gpointer, GCancellable *) {
  std::call_once(curlInitFlag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

  auto *result = new UpdateChecker::Result();
  CURL *curl = curl_easy_init();
  if (!curl) {
    result->errorMessage = "Unable to initialize the network client.";
    g_task_return_pointer(task, result,
                          [](gpointer data) {
                            delete static_cast<UpdateChecker::Result *>(data);
                          });
    return;
  }

  std::string response;
  char errorBuffer[CURL_ERROR_SIZE] = {};
  curl_easy_setopt(curl, CURLOPT_URL, VELNIX_UPDATE_URL);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "VelnixEditor/" VELNIX_EDITOR_VERSION);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

  const CURLcode code = curl_easy_perform(curl);
  long statusCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
  curl_easy_cleanup(curl);

  if (code != CURLE_OK) {
    result->errorMessage = errorBuffer[0] ? errorBuffer : curl_easy_strerror(code);
  } else if (statusCode != 200) {
    result->errorMessage = "The update server returned HTTP " +
                           std::to_string(statusCode) + ".";
  } else {
    result->latestVersion = trim(response);
    if (parse_version(result->latestVersion).empty()) {
      result->errorMessage = "The update server returned an invalid version.";
    } else {
      result->success = true;
    }
  }

  g_task_return_pointer(task, result,
                        [](gpointer data) {
                          delete static_cast<UpdateChecker::Result *>(data);
                        });
}

void finish_check(GObject *, GAsyncResult *asyncResult, gpointer data) {
  std::unique_ptr<AsyncRequest> request(static_cast<AsyncRequest *>(data));
  auto *result = static_cast<UpdateChecker::Result *>(
      g_task_propagate_pointer(G_TASK(asyncResult), nullptr));
  if (result) {
    request->callback(std::move(*result));
    delete result;
  }
}

}  // namespace

void UpdateChecker::checkAsync(Callback callback) {
  auto *request = new AsyncRequest{std::move(callback)};
  GTask *task = g_task_new(nullptr, nullptr, finish_check, request);
  g_task_run_in_thread(task, run_check);
  g_object_unref(task);
}

bool UpdateChecker::isNewerVersion(const std::string &candidate,
                                   const std::string &current) {
  std::vector<int> candidateParts = parse_version(candidate);
  std::vector<int> currentParts = parse_version(current);
  if (candidateParts.empty() || currentParts.empty()) {
    return false;
  }
  const size_t count = std::max(candidateParts.size(), currentParts.size());
  candidateParts.resize(count, 0);
  currentParts.resize(count, 0);
  return candidateParts > currentParts;
}
