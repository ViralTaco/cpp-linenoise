
#include <typeinfo>

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct State {
  int ifd;            /* Terminal stdin file descriptor. */
  int ofd;            /* Terminal stdout file descriptor. */
  char* buf;          /* Edited line buffer. */
  int buflen;         /* Edited line buffer size. */
  std::string prompt; /* Prompt to display. */
  int pos;            /* Current cursor position. */
  int oldcolpos;      /* Previous refresh cursor column position. */
  int len;            /* Current edited line length. */
  int cols;           /* Number of columns in terminal. */
  int maxrows;        /* Maximum num of rows used so far (multiline mode) */
  int history_index;  /* The history index we are currently editing. */
};

enum KEY_ACTION {
  KEY_NULL = 0,   /* NULL */
  CTRL_A = 1,     /* Ctrl+a */
  CTRL_B = 2,     /* Ctrl-b */
  CTRL_C = 3,     /* Ctrl-c */
  CTRL_D = 4,     /* Ctrl-d */
  CTRL_E = 5,     /* Ctrl-e */
  CTRL_F = 6,     /* Ctrl-f */
  CTRL_H = 8,     /* Ctrl-h */
  TAB = 9,        /* Tab */
  CTRL_K = 11,    /* Ctrl+k */
  CTRL_L = 12,    /* Ctrl+l */
  ENTER = 13,     /* Enter */
  CTRL_N = 14,    /* Ctrl-n */
  CTRL_P = 16,    /* Ctrl-p */
  CTRL_T = 20,    /* Ctrl-t */
  CTRL_U = 21,    /* Ctrl+u */
  CTRL_W = 23,    /* Ctrl+w */
  ESC = 27,       /* Escape */
  BACKSPACE = 127 /* Backspace */
};

void at_exit();
bool AddHistory(const char* line);
void refreshLine(struct State* l);

/* ============================ UTF8 utilities ============================== */


static int unicodeWideCharTableSize =
    sizeof(kUnicodeWideChars) / sizeof(kUnicodeWideChars[0]);

static int unicodeIsWideChar(unsigned long cp) {
  int i;
  for (i = 0; i < unicodeWideCharTableSize; i++) {
    if (kUnicodeWideChars[i][0] <= cp && cp <= kUnicodeWideChars[i][1]) {
      return 1;
    }
  }
  return 0;
}

static int unicodeCombiningCharTableSize =
  sizeof(kUnicodeCombiningChar) / sizeof(kUnicodeCombiningChar[0]);

inline int unicodeIsCombiningChar(unsigned long cp) {
  int i;
  for (i = 0; i < unicodeCombiningCharTableSize; i++) {
    if (kUnicodeCombiningChar[i] == cp) {
      return 1;
    }
  }
  return 0;
}

/* Get length of previous UTF8 character
 */
inline int unicodePrevUTF8CharLen(char* buf, int pos) {
  int end = pos--;
  while (pos >= 0 && ((unsigned char)buf[pos] & 0xC0) == 0x80) {
    pos--;
  }
  return end - pos;
}

/* Get length of previous UTF8 character
 */
int utf8_char_length(char const* buf, const int buf_len, const int pos) noexcept {
  if (pos == buf_len) { return 0; }
  if (const auto ch = static_cast<unsigned> (buf[pos])) {
    if (ch < 0x80) {
      return 1;
    } else if (ch < 0xE0) {
      return 2;
    } else if (ch < 0xF0) {
      return 3;
    }
  }
    return 4;
}

/* Convert UTF8 to Unicode code point
 */
inline int unicodeUTF8CharToCodePoint(const char* buf, int len, int* cp) {
  if (len) {
    unsigned char byte = buf[0];
    if ((byte & 0x80) == 0) {
      *cp = byte;
      return 1;
    } else if ((byte & 0xE0) == 0xC0) {
      if (len >= 2) {
        *cp = (((unsigned long)(buf[0] & 0x1F)) << 6) |
              ((unsigned long)(buf[1] & 0x3F));
        return 2;
      }
    } else if ((byte & 0xF0) == 0xE0) {
      if (len >= 3) {
        *cp = (((unsigned long)(buf[0] & 0x0F)) << 12) |
              (((unsigned long)(buf[1] & 0x3F)) << 6) |
              ((unsigned long)(buf[2] & 0x3F));
        return 3;
      }
    } else if ((byte & 0xF8) == 0xF0) {
      if (len >= 4) {
        *cp = (((unsigned long)(buf[0] & 0x07)) << 18) |
              (((unsigned long)(buf[1] & 0x3F)) << 12) |
              (((unsigned long)(buf[2] & 0x3F)) << 6) |
              ((unsigned long)(buf[3] & 0x3F));
        return 4;
      }
    }
  }
  return 0;
}

/* Get length of grapheme
 */
inline int unicodeGraphemeLen(char* buf, int buf_len, int pos) {
  if (pos == buf_len) {
    return 0;
  }
  int beg = pos;
  pos += utf8_char_length(buf, buf_len, pos);
  while (pos < buf_len) {
    int len = utf8_char_length(buf, buf_len, pos);
    int cp = 0;
    unicodeUTF8CharToCodePoint(buf + pos, len, &cp);
    if (!unicodeIsCombiningChar(cp)) {
      return pos - beg;
    }
    pos += len;
  }
  return pos - beg;
}

/* Get length of previous grapheme
 */
inline int unicodePrevGraphemeLen(char* buf, int pos) {
  if (pos == 0) {
    return 0;
  }
  int end = pos;
  while (pos > 0) {
    int len = unicodePrevUTF8CharLen(buf, pos);
    pos -= len;
    int cp = 0;
    unicodeUTF8CharToCodePoint(buf + pos, len, &cp);
    if (!unicodeIsCombiningChar(cp)) {
      return end - pos;
    }
  }
  return 0;
}

bool is_ansi_escape(std::string_view buf, int& len) {
  constexpr auto kEscape = "ABCDEFGHJKSTfm"sv;
  if (buf.starts_with("\033[")) {
    for (int offset = 2; offset < buf.length(); ++offset) {
      if (std::any_of(kEscape.begin(), kEscape.end(),
        [&] (const auto c) { return buf[offset] == c; })
      ) {
        len = offset;
        return true;
      }
    }
  }
  return false;
}


/* Get column position for the single line mode.
 */
inline int unicode_column_pos(const char* buf, int buf_len) {
  int column_pos = 0;
  
  for (int len = 0, off = 0; off < buf_len; off += len) {
    if (is_ansi_escape(buf + off, len)) {
      off += len;
      continue;
    }

    int cp = 0;
    len = unicodeUTF8CharToCodePoint(buf + off, buf_len - off, &cp);

    if (!unicodeIsCombiningChar(cp)) {
      column_pos += unicodeIsWideChar(cp) ? 2 : 1;
    }
  }

  
  return column_pos;
}

/* Get column position for the multi line mode.
 */
inline int unicodeColumnPosForMultiLine(char* buf, int buf_len, int pos,
                                        int cols, int ini_pos) {
  int ret = 0;
  int colwid = ini_pos;

  int off = 0;
  while (off < buf_len) {
    int cp = 0;
    int len = unicodeUTF8CharToCodePoint(buf + off, buf_len - off, &cp);

    int wid = 0;
    if (!unicodeIsCombiningChar(cp)) {
      wid = unicodeIsWideChar(cp) ? 2 : 1;
    }

    int dif = (int)(colwid + wid) - (int)cols;
    if (dif > 0) {
      ret += dif;
      colwid = wid;
    } else if (dif == 0) {
      colwid = 0;
    } else {
      colwid += wid;
    }

    if (off >= pos) {
      break;
    }

    off += len;
    ret += wid;
  }

  return ret;
}

/* Read UTF8 character from file.
 */
inline int unicodeReadUTF8Char(int fd, char* buf, int* cp) {
  int nread = read(fd, &buf[0], 1);

  if (nread <= 0) {
    return nread;
  }

  unsigned char byte = buf[0];

  if ((byte & 0x80) == 0) {
    ;
  } else if ((byte & 0xE0) == 0xC0) {
    nread = read(fd, &buf[1], 1);
    if (nread <= 0) {
      return nread;
    }
  } else if ((byte & 0xF0) == 0xE0) {
    nread = read(fd, &buf[1], 2);
    if (nread <= 0) {
      return nread;
    }
  } else if ((byte & 0xF8) == 0xF0) {
    nread = read(fd, &buf[1], 3);
    if (nread <= 0) {
      return nread;
    }
  } else {
    return -1;
  }

  return unicodeUTF8CharToCodePoint(buf, 4, cp);
}

/* ======================= Low level terminal handling ====================== */

/* Set if to use or not the multi line mode. */
inline void SetMultiLine(bool ml) { mlmode = ml; }

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
inline bool is_unsupported_term() {
#ifndef _WIN32
  if (const auto env_term = std::getenv("TERM"); env_term != nullptr) {
    for (const auto term: kUnsupportedTerms) {
      if (term == env_term) { return true; }
    }
  }
#endif
  return false;
}

/* Raw mode: 1960 magic shit. */
inline bool use_raw(int fd) {
  struct termios raw;

  if (!isatty(STDIN_FILENO)) {
    goto fatal;
  }
  if (!atexit_registered) {
    at_exit();
    atexit_registered = true;
  }
  if (tcgetattr(fd, &orig_termios) == -1) {
    goto fatal;
  }

  raw = orig_termios; /* modify the original mode */
  /* input modes: no break, no CR to NL, no parity check, no strip char,
   * no start/stop output control. */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  /* output modes - disable post processing */
  // NOTE: Multithreaded issue #20
  // (https://github.com/yhirose/cpp-linenoise/issues/20) raw.c_oflag &=
  // ~(OPOST);
  /* control modes - set 8 bit chars */
  raw.c_cflag |= (CS8);
  /* local modes - echoing off, canonical off, no extended functions,
   * no signal chars (^Z,^C) */
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  /* control chars - set return condition: min number of bytes and timer.
   * We want read to return every single byte, without timeout. */
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

  /* put terminal in raw mode after flushing */
  if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) {
    goto fatal;
  }
  rawmode = true;

  return true;

fatal:
  errno = ENOTTY;
  return false;
}

inline void disableRawMode(int fd) {
#ifdef _WIN32
  if (consolemodeIn) {
    SetConsoleMode(hIn, consolemodeIn);
    consolemodeIn = 0;
  }
  rawmode = false;
#else
  /* Don't even check the return value as it's too late. */
  if (rawmode && tcsetattr(fd, TCSAFLUSH, &orig_termios) != -1) {
    rawmode = false;
  }
#endif
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
inline int getCursorPosition(int ifd, int ofd) {
  char buf[32];
  int cols, rows;
  unsigned int i = 0;

  /* Report cursor location */
  if (write(ofd, "\x1b[6n", 4) != 4) {
    return -1;
  }

  /* Read the response: ESC [ rows ; cols R */
  while (i < sizeof(buf) - 1) {
    if (read(ifd, buf + i, 1) != 1) {
      break;
    }
    if (buf[i] == 'R') {
      break;
    }
    i++;
  }
  buf[i] = '\0';

  /* Parse it. */
  if (buf[0] != ESC || buf[1] != '[') {
    return -1;
  }
  if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2) {
    return -1;
  }
  return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
inline int getColumns(int ifd, int ofd) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO b;

  if (!GetConsoleScreenBufferInfo(hOut, &b)) return 80;
  return b.srWindow.Right - b.srWindow.Left;
#else
  struct winsize ws;

  if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    /* ioctl() failed. Try to query the terminal itself. */
    int start, cols;

    /* Get the initial position so we can restore it later. */
    start = getCursorPosition(ifd, ofd);
    if (start == -1) {
      goto failed;
    }

    /* Go to right margin and get position. */
    if (write(ofd, "\x1b[999C", 6) != 6) {
      goto failed;
    }
    cols = getCursorPosition(ifd, ofd);
    if (cols == -1) {
      goto failed;
    }

    /* Restore position. */
    if (cols > start) {
      char seq[32];
      snprintf(seq, 32, "\x1b[%dD", cols - start);
      if (write(ofd, seq, strlen(seq)) == -1) {
        /* Can't recover... */
      }
    }
    return cols;
  } else {
    return ws.ws_col;
  }

failed:
  return 80;
#endif
}

/* Clear the screen. Used to handle ctrl+l */
inline void linenoiseClearScreen(void) {
  if (write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7) <= 0) {
    /* nothing to do, just to avoid warning. */
  }
}

/* Beep, used for completion when there is nothing to complete or when all
 * the choices were already shown. */
inline void linenoiseBeep(void) {
  fprintf(stderr, "\x7");
  fflush(stderr);
}

/* ============================== Completion ================================ */

/* This is an helper function for linenoiseEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition. */
inline int completeLine(struct State* ls, char* cbuf, int* c) {
  std::vector<std::string> lc;
  int nread = 0, nwritten;
  *c = 0;

  completionCallback(ls->buf, lc);
  if (lc.empty()) {
    linenoiseBeep();
  } else {
    int stop = 0, i = 0;

    while (!stop) {
      /* Show completion or original buffer */
      if (i < static_cast<int>(lc.size())) {
        struct State saved = *ls;

        ls->len = ls->pos = static_cast<int>(lc[i].size());
        ls->buf = &lc[i][0];
        refreshLine(ls);
        ls->len = saved.len;
        ls->pos = saved.pos;
        ls->buf = saved.buf;
      } else {
        refreshLine(ls);
      }

      // nread = read(ls->ifd,&c,1);
#ifdef _WIN32
      nread = win32read(c);
      if (nread == 1) {
        cbuf[0] = *c;
      }
#else
      nread = unicodeReadUTF8Char(ls->ifd, cbuf, c);
#endif
      if (nread <= 0) {
        *c = -1;
        return nread;
      }

      switch (*c) {
        case 9: /* tab */
          i = (i + 1) % (lc.size() + 1);
          if (i == static_cast<int>(lc.size())) {
            linenoiseBeep();
          }
          break;
        case 27: /* escape */
          /* Re-show original buffer */
          if (i < static_cast<int>(lc.size())) {
            refreshLine(ls);
          }
          stop = 1;
          break;
        default:
          /* Update buffer and return */
          if (i < static_cast<int>(lc.size())) {
            nwritten = snprintf(ls->buf, ls->buflen, "%s", &lc[i][0]);
            ls->len = ls->pos = nwritten;
          }
          stop = 1;
          break;
      }
    }
  }

  return nread;
}

/* Register a callback function to be called for tab-completion. */
inline void SetCompletionCallback(CompletionCallback fn) {
  completionCallback = fn;
}

/* =========================== Line editing ================================= */

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
inline void refreshSingleLine(struct State* l) {
  char seq[64];
  int pcolwid =
    unicode_column_pos(
      l->prompt
       .c_str(), static_cast<int>(l->prompt
                                   .length())
    );
  int fd = l->ofd;
  char* buf = l->buf;
  int len = l->len;
  int pos = l->pos;
  std::string ab;

  while ((pcolwid + unicode_column_pos(buf, pos)) >= l->cols) {
    int glen = unicodeGraphemeLen(buf, len, 0);
    buf += glen;
    len -= glen;
    pos -= glen;
  }
  while (pcolwid + unicode_column_pos(buf, len) > l->cols) {
    len -= unicodePrevGraphemeLen(buf, len);
  }

  /* Cursor to left edge */
  snprintf(seq, 64, "\r");
  ab += seq;
  /* Write the prompt and the current buffer content */
  ab += l->prompt;
  ab.append(buf, len);
  /* Erase to right */
  snprintf(seq, 64, "\x1b[0K");
  ab += seq;
  /* Move cursor to original position. */
  snprintf(seq, 64, "\r\x1b[%dC", (int)(unicode_column_pos(buf, pos) + pcolwid));
  ab += seq;
  if (write(fd, ab.c_str(), static_cast<int>(ab.length())) == -1) {
  } /* Can't recover from write error. */
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
inline void refreshMultiLine(struct State* l) {
  char seq[64];
  int pcolwid =
    unicode_column_pos(
      l->prompt
       .c_str(), static_cast<int>(l->prompt
                                   .length())
    );
  int colpos =
      unicodeColumnPosForMultiLine(l->buf, l->len, l->len, l->cols, pcolwid);
  int colpos2; /* cursor column position. */
  int rows = (pcolwid + colpos + l->cols - 1) /
             l->cols; /* rows used by current buf. */
  int rpos =
      (pcolwid + l->oldcolpos + l->cols) / l->cols; /* cursor relative row. */
  int rpos2;                                        /* rpos after refresh. */
  int col; /* colum position, zero-based. */
  int old_rows = (int)l->maxrows;
  int fd = l->ofd, j;
  std::string ab;

  /* Update maxrows if needed. */
  if (rows > (int)l->maxrows) {
    l->maxrows = rows;
  }

  /* First step: clear all the lines used before. To do so start by
   * going to the last row. */
  if (old_rows - rpos > 0) {
    snprintf(seq, 64, "\x1b[%dB", old_rows - rpos);
    ab += seq;
  }

  /* Now for every row clear it, go up. */
  for (j = 0; j < old_rows - 1; j++) {
    snprintf(seq, 64, "\r\x1b[0K\x1b[1A");
    ab += seq;
  }

  /* Clean the top line. */
  snprintf(seq, 64, "\r\x1b[0K");
  ab += seq;

  /* Write the prompt and the current buffer content */
  ab += l->prompt;
  ab.append(l->buf, l->len);

  /* Get text width to cursor position */
  colpos2 =
      unicodeColumnPosForMultiLine(l->buf, l->len, l->pos, l->cols, pcolwid);

  /* If we are at the very end of the screen with our prompt, we need to
   * emit a newline and move the prompt to the first column. */
  if (l->pos && l->pos == l->len && (colpos2 + pcolwid) % l->cols == 0) {
    ab += "\n";
    snprintf(seq, 64, "\r");
    ab += seq;
    rows++;
    if (rows > (int)l->maxrows) {
      l->maxrows = rows;
    }
  }

  /* Move cursor to right position. */
  rpos2 = (pcolwid + colpos2 + l->cols) /
          l->cols; /* current cursor relative row. */

  /* Go up till we reach the expected positon. */
  if (rows - rpos2 > 0) {
    snprintf(seq, 64, "\x1b[%dA", rows - rpos2);
    ab += seq;
  }

  /* Set column. */
  col = (pcolwid + colpos2) % l->cols;
  if (col) {
    snprintf(seq, 64, "\r\x1b[%dC", col);
  } else {
    snprintf(seq, 64, "\r");
  }
  ab += seq;

  l->oldcolpos = colpos2;

  if (write(fd, ab.c_str(), static_cast<int>(ab.length())) == -1) {
  } /* Can't recover from write error. */
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
inline void refreshLine(struct State* l) {
  if (mlmode) {
    refreshMultiLine(l);
  } else {
    refreshSingleLine(l);
  }
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
inline int linenoiseEditInsert(struct State* l, const char* cbuf,
                               int clen) {
  if (l->len < l->buflen) {
    if (l->len == l->pos) {
      memcpy(&l->buf[l->pos], cbuf, clen);
      l->pos += clen;
      l->len += clen;
      ;
      l->buf[l->len] = '\0';
      if ((!mlmode && unicode_column_pos(
        l->prompt
         .c_str(),
        static_cast<int>(l->prompt
                          .length())
      ) +
                      unicode_column_pos(
                                                                               l->buf,
                                                                               l->len
                                                                             ) <
                      l->cols) /* || mlmode */) {
        /* Avoid a full update of the line in the
         * trivial case. */
        if (write(l->ofd, cbuf, clen) == -1) {
          return -1;
        }
      } else {
        refreshLine(l);
      }
    } else {
      memmove(l->buf + l->pos + clen, l->buf + l->pos, l->len - l->pos);
      memcpy(&l->buf[l->pos], cbuf, clen);
      l->pos += clen;
      l->len += clen;
      l->buf[l->len] = '\0';
      refreshLine(l);
    }
  }
  return 0;
}

/* Move cursor on the left. */
inline void linenoiseEditMoveLeft(struct State* l) {
  if (l->pos > 0) {
    l->pos -= unicodePrevGraphemeLen(l->buf, l->pos);
    refreshLine(l);
  }
}

/* Move cursor on the right. */
inline void linenoiseEditMoveRight(struct State* l) {
  if (l->pos != l->len) {
    l->pos += unicodeGraphemeLen(l->buf, l->len, l->pos);
    refreshLine(l);
  }
}

/* Move cursor to the start of the line. */
inline void linenoiseEditMoveHome(struct State* l) {
  if (l->pos != 0) {
    l->pos = 0;
    refreshLine(l);
  }
}

/* Move cursor to the end of the line. */
inline void linenoiseEditMoveEnd(struct State* l) {
  if (l->pos != l->len) {
    l->pos = l->len;
    refreshLine(l);
  }
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1

inline void linenoiseEditHistoryNext(struct State* l, int dir) {
  if (history.size() > 1) {
    /* Update the current history entry before to
     * overwrite it with the next one. */
    history[history.size() - 1 - l->history_index] = l->buf;
    /* Show the new entry */
    l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
    if (l->history_index < 0) {
      l->history_index = 0;
      return;
    } else if (l->history_index >= (int)history.size()) {
      l->history_index = static_cast<int>(history.size()) - 1;
      return;
    }
    memset(l->buf, 0, l->buflen);
    strcpy(l->buf, history[history.size() - 1 - l->history_index].c_str());
    l->len = l->pos = static_cast<int>(strlen(l->buf));
    refreshLine(l);
  }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
inline void linenoiseEditDelete(struct State* l) {
  if (l->len > 0 && l->pos < l->len) {
    int glen = unicodeGraphemeLen(l->buf, l->len, l->pos);
    memmove(l->buf + l->pos, l->buf + l->pos + glen, l->len - l->pos - glen);
    l->len -= glen;
    l->buf[l->len] = '\0';
    refreshLine(l);
  }
}

/* Backspace implementation. */
inline void linenoiseEditBackspace(struct State* l) {
  if (l->pos > 0 && l->len > 0) {
    int glen = unicodePrevGraphemeLen(l->buf, l->pos);
    memmove(l->buf + l->pos - glen, l->buf + l->pos, l->len - l->pos);
    l->pos -= glen;
    l->len -= glen;
    l->buf[l->len] = '\0';
    refreshLine(l);
  }
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
inline void linenoiseEditDeletePrevWord(struct State* l) {
  int old_pos = l->pos;
  int diff;

  while (l->pos > 0 && l->buf[l->pos - 1] == ' ') l->pos--;
  while (l->pos > 0 && l->buf[l->pos - 1] != ' ') l->pos--;
  diff = old_pos - l->pos;
  memmove(l->buf + l->pos, l->buf + old_pos, l->len - old_pos + 1);
  l->len -= diff;
  refreshLine(l);
}

/* This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
inline int linenoiseEdit(int stdin_fd, int stdout_fd, char* buf, int buflen,
                         const char* prompt) {
  struct State l;

  /* Populate the linenoise state that we pass to functions implementing
   * specific editing functionalities. */
  l.ifd = stdin_fd;
  l.ofd = stdout_fd;
  l.buf = buf;
  l.buflen = buflen;
  l.prompt = prompt;
  l.oldcolpos = l.pos = 0;
  l.len = 0;
  l.cols = getColumns(stdin_fd, stdout_fd);
  l.maxrows = 0;
  l.history_index = 0;

  /* Buffer starts empty. */
  l.buf[0] = '\0';
  l.buflen--; /* Make sure there is always space for the nulterm */

  /* The latest history entry is always our current buffer, that
   * initially is just an empty string. */
  AddHistory("");

  if (write(l.ofd, prompt, static_cast<int>(l.prompt.length())) == -1) {
    return -1;
  }
  while (1) {
    int c;
    char cbuf[4];
    int nread;
    char seq[3];


    nread = unicodeReadUTF8Char(l.ifd, cbuf, &c);
    if (nread <= 0) {
      return (int)l.len;
    }

    /* Only autocomplete when the callback is set. It returns < 0 when
     * there was an error reading from fd. Otherwise it will return the
     * character that should be handled next. */
    if (c == 9 && completionCallback != NULL) {
      nread = completeLine(&l, cbuf, &c);
      /* Return on errors */
      if (c < 0) {
        return l.len;
      }
      /* Read next character when 0 */
      if (c == 0) {
        continue;
      }
    }

    switch (c) {
      case ENTER: /* enter */
        if (!history.empty()) {
          history.pop_back();
        }
        if (mlmode) {
          linenoiseEditMoveEnd(&l);
        }
        return (int)l.len;
      case CTRL_C: /* ctrl-c */
        errno = EAGAIN;
        return -1;
      case BACKSPACE: /* backspace */
      case 8:         /* ctrl-h */
        linenoiseEditBackspace(&l);
        break;
      case CTRL_D: /* ctrl-d, remove char at right of cursor, or if the
                      line is empty, act as end-of-file. */
        if (l.len > 0) {
          linenoiseEditDelete(&l);
        } else {
          history.pop_back();
          return -1;
        }
        break;
      case CTRL_T: /* ctrl-t, swaps current character with previous. */
        if (l.pos > 0 && l.pos < l.len) {
          char aux = buf[l.pos - 1];
          buf[l.pos - 1] = buf[l.pos];
          buf[l.pos] = aux;
          if (l.pos != l.len - 1) {
            l.pos++;
          }
          refreshLine(&l);
        }
        break;
      case CTRL_B: /* ctrl-b */
        linenoiseEditMoveLeft(&l);
        break;
      case CTRL_F: /* ctrl-f */
        linenoiseEditMoveRight(&l);
        break;
      case CTRL_P: /* ctrl-p */
        linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
        break;
      case CTRL_N: /* ctrl-n */
        linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
        break;
      case ESC: /* escape sequence */
        /* Read the next two bytes representing the escape sequence.
         * Use two calls to handle slow terminals returning the two
         * chars at different times. */
        if (read(l.ifd, seq, 1) == -1) {
          break;
        }
        if (read(l.ifd, seq + 1, 1) == -1) {
          break;
        }

        /* ESC [ sequences. */
        if (seq[0] == '[') {
          if (seq[1] >= '0' && seq[1] <= '9') {
            /* Extended escape, read additional byte. */
            if (read(l.ifd, seq + 2, 1) == -1) {
              break;
            }
            if (seq[2] == '~') {
              switch (seq[1]) {
                case '3': /* Delete key. */
                  linenoiseEditDelete(&l);
                  break;
              }
            }
          } else {
            switch (seq[1]) {
              case 'A': /* Up */
                linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
                break;
              case 'B': /* Down */
                linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
                break;
              case 'C': /* Right */
                linenoiseEditMoveRight(&l);
                break;
              case 'D': /* Left */
                linenoiseEditMoveLeft(&l);
                break;
              case 'H': /* Home */
                linenoiseEditMoveHome(&l);
                break;
              case 'F': /* End*/
                linenoiseEditMoveEnd(&l);
                break;
            }
          }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
          switch (seq[1]) {
            case 'H': /* Home */
              linenoiseEditMoveHome(&l);
              break;
            case 'F': /* End*/
              linenoiseEditMoveEnd(&l);
              break;
          }
        }
        break;
      default:
        if (linenoiseEditInsert(&l, cbuf, nread)) {
          return -1;
        }
        break;
      case CTRL_U: /* Ctrl+u, delete the whole line. */
        buf[0] = '\0';
        l.pos = l.len = 0;
        refreshLine(&l);
        break;
      case CTRL_K: /* Ctrl+k, delete from current to end of line. */
        buf[l.pos] = '\0';
        l.len = l.pos;
        refreshLine(&l);
        break;
      case CTRL_A: /* Ctrl+a, go to the start of the line */
        linenoiseEditMoveHome(&l);
        break;
      case CTRL_E: /* ctrl+e, go to the end of the line */
        linenoiseEditMoveEnd(&l);
        break;
      case CTRL_L: /* ctrl+l, clear screen */
        linenoiseClearScreen();
        refreshLine(&l);
        break;
      case CTRL_W: /* ctrl+w, delete previous word */
        linenoiseEditDeletePrevWord(&l);
        break;
    }
  }
  return l.len;
}

/* This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode. */
inline bool linenoiseRaw(const char* prompt, std::string& line) {
  bool quit = false;

  if (!isatty(STDIN_FILENO)) {
    /* Not a tty: read from file / pipe. */
    std::getline(std::cin, line);
  } else {
    /* Interactive editing. */
    if (not use_raw(STDIN_FILENO)) {
      return quit;
    }

    char buf[LINENOISE_MAX_LINE];
    auto count = linenoiseEdit(STDIN_FILENO, STDOUT_FILENO, buf,
                               LINENOISE_MAX_LINE, prompt);
    if (count == -1) {
      quit = true;
    } else {
      line.assign(buf, count);
    }

    disableRawMode(STDIN_FILENO);
    printf("\n");
  }
  return quit;
}

/* The high level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions. */
inline bool Readline(const char* prompt, std::string& line) {
  if (is_unsupported_term()) {
    printf("%s", prompt);
    fflush(stdout);
    std::getline(std::cin, line);
    return false;
  } else {
    return linenoiseRaw(prompt, line);
  }
}

inline std::string Readline(const char* prompt, bool& quit) {
  std::string line;
  quit = Readline(prompt, line);
  return line;
}

inline std::string Readline(const char* prompt) {
  bool quit;  // dummy
  return Readline(prompt, quit);
}

/* ================================ History ================================= */

/* At exit we'll try to fix the terminal to the initial conditions. */
inline void at_exit() {
    disableRawMode(STDIN_FILENO);
    
}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
inline bool AddHistory(const char* line) {
  if (history_max_len == 0) {
    return false;
  }

  /* Don't add duplicated lines. */
  if (!history.empty() && history.back() == line) {
    return false;
  }

  /* If we reached the max length, remove the older line. */
  if (history.size() == history_max_len) {
    history.erase(history.begin());
  }
  history.push_back(line);

  return true;
}

/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history. */
inline bool SetHistoryMaxLen(size_t len) {
  if (len < 1) {
    return false;
  }
  history_max_len = len;
  if (len < history.size()) {
    history.resize(len);
  }
  return true;
}

/* Save the history in the specified file. On success *true* is returned
 * otherwise *false* is returned. */
inline bool SaveHistory(const char* path) {
  std::ofstream f(path);  // TODO: need 'std::ios::binary'?
  if (!f) {
    return false;
  }
  for (const auto& h : history) {
    f << h << std::endl;
  }
  return true;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded *true* is returned, otherwise
 * on error *false* is returned. */
inline bool LoadHistory(const char* path) {
  std::ifstream f(path);
  if (!f) {
    return false;
  }
  std::string line;
  while (std::getline(f, line)) {
    AddHistory(line.c_str());
  }
  return true;
}

inline const std::vector<std::string>& GetHistory() { return history; }

}  // namespace linenoise

#endif /* __LINENOISE_HPP */
