#include "core/EditorTypes.h"

RecordedMacroStep::RecordedMacroStep(int iMessage, uptr_t wParam, sptr_t lParam)
  : message(iMessage), wParameter(wParam), lParameter(lParam) {}

bool RecordedMacroStep::isMacroable() const {
  if (macroType == MacroTypeIndex::mtAppCommand) {
    return true;
  }

  // Enumerates all macroable messages
  switch (message) {
    case SCI_REPLACESEL: // (<unused>, const char *text)
    case SCI_ADDTEXT:    // (int length, const char *s)
    case SCI_INSERTTEXT: // (int pos, const char *text)
    case SCI_APPENDTEXT: // (int length, const char *s)
    case SCI_SEARCHNEXT: // (int searchFlags, const char *text)
    case SCI_SEARCHPREV: // (int searchFlags, const char *text)
    {
      if (macroType == MacroTypeIndex::mtUseSParameter)
        return true;
      else
        return false;
    }

    case SCI_GOTOLINE:   // (int line)
    case SCI_GOTOPOS:    // (int position)
    case SCI_DELETERANGE: // (position startDelete, int deleteLength)
    case SCI_SETSELECTIONMODE:  // (int mode)
    case SCI_CUT:
    case SCI_COPY:
    case SCI_PASTE:
    case SCI_CLEAR:
    case SCI_CLEARALL:
    case SCI_SELECTALL:
    case SCI_SEARCHANCHOR:
    case SCI_LINEDOWN:
    case SCI_LINEDOWNEXTEND:
    case SCI_PARADOWN:
    case SCI_PARADOWNEXTEND:
    case SCI_LINEUP:
    case SCI_LINEUPEXTEND:
    case SCI_PARAUP:
    case SCI_PARAUPEXTEND:
    case SCI_CHARLEFT:
    case SCI_CHARLEFTEXTEND:
    case SCI_CHARRIGHT:
    case SCI_CHARRIGHTEXTEND:
    case SCI_WORDLEFT:
    case SCI_WORDLEFTEXTEND:
    case SCI_WORDRIGHT:
    case SCI_WORDRIGHTEXTEND:
    case SCI_WORDPARTLEFT:
    case SCI_WORDPARTLEFTEXTEND:
    case SCI_WORDPARTRIGHT:
    case SCI_WORDPARTRIGHTEXTEND:
    case SCI_WORDLEFTEND:
    case SCI_WORDLEFTENDEXTEND:
    case SCI_WORDRIGHTEND:
    case SCI_WORDRIGHTENDEXTEND:
    case SCI_HOME:
    case SCI_HOMEEXTEND:
    case SCI_LINEEND:
    case SCI_LINEENDEXTEND:
    case SCI_HOMEWRAP:
    case SCI_HOMEWRAPEXTEND:
    case SCI_LINEENDWRAP:
    case SCI_LINEENDWRAPEXTEND:
    case SCI_DOCUMENTSTART:
    case SCI_DOCUMENTSTARTEXTEND:
    case SCI_DOCUMENTEND:
    case SCI_DOCUMENTENDEXTEND:
    case SCI_STUTTEREDPAGEUP:
    case SCI_STUTTEREDPAGEUPEXTEND:
    case SCI_STUTTEREDPAGEDOWN:
    case SCI_STUTTEREDPAGEDOWNEXTEND:
    case SCI_PAGEUP:
    case SCI_PAGEUPEXTEND:
    case SCI_PAGEDOWN:
    case SCI_PAGEDOWNEXTEND:
    case SCI_EDITTOGGLEOVERTYPE:
    case SCI_CANCEL:
    case SCI_DELETEBACK:
    case SCI_TAB:
    case SCI_BACKTAB:
    case SCI_FORMFEED:
    case SCI_VCHOME:
    case SCI_VCHOMEEXTEND:
    case SCI_VCHOMEWRAP:
    case SCI_VCHOMEWRAPEXTEND:
    case SCI_VCHOMEDISPLAY:
    case SCI_VCHOMEDISPLAYEXTEND:
    case SCI_DELWORDLEFT:
    case SCI_DELWORDRIGHT:
    case SCI_DELWORDRIGHTEND:
    case SCI_DELLINELEFT:
    case SCI_DELLINERIGHT:
    case SCI_LINECOPY:
    case SCI_LINECUT:
    case SCI_LINEDELETE:
    case SCI_LINETRANSPOSE:
    case SCI_LINEDUPLICATE:
    case SCI_LOWERCASE:
    case SCI_UPPERCASE:
    case SCI_LINESCROLLDOWN:
    case SCI_LINESCROLLUP:
    case SCI_DELETEBACKNOTLINE:
    case SCI_HOMEDISPLAY:
    case SCI_HOMEDISPLAYEXTEND:
    case SCI_LINEENDDISPLAY:
    case SCI_LINEENDDISPLAYEXTEND:
    case SCI_LINEDOWNRECTEXTEND:
    case SCI_LINEUPRECTEXTEND:
    case SCI_CHARLEFTRECTEXTEND:
    case SCI_CHARRIGHTRECTEXTEND:
    case SCI_HOMERECTEXTEND:
    case SCI_VCHOMERECTEXTEND:
    case SCI_LINEENDRECTEXTEND:
    case SCI_PAGEUPRECTEXTEND:
    case SCI_PAGEDOWNRECTEXTEND:
    case SCI_SELECTIONDUPLICATE:
    case SCI_COPYALLOWLINE:
    case SCI_VERTICALCENTRECARET:
    case SCI_MOVESELECTEDLINESUP:
    case SCI_MOVESELECTEDLINESDOWN:
    case SCI_SCROLLTOSTART:
    case SCI_SCROLLTOEND:
    case SCI_SETVIRTUALSPACEOPTIONS:
    case SCI_SETCARETLINEBACKALPHA:
    case SCI_NEWLINE:
    {
      if (macroType == MacroTypeIndex::mtUseLParameter)
        return true;
      else
        return false;
    }

    // Filter out all others like display changes.
    default:
      return false;
  }
}
