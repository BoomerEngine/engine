package utils;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class CodeTokenizer {

  public static List<CodeToken> tokenize(String txt) {
    List<CodeToken> ret = new ArrayList<>();

    // create tokenization state
    State s = new State(txt);

    // high-level parser loop
    while (s.hasContent()) {
      char ch = s.peekChar();

      if (ch == '/') {
        handleComment(s);
      } else if (ch == '\"' || ch == '\'') {
        ret.add(handleString(s));
      } else if (ch <= ' ') {
        s.eatChar(); // whitespace
      } else if (ch <= '#') {
        handlePreprocesor(s);
      } else if (isTokenChar(ch)) {
        ret.add(handleIdent(s));
      } else {
        ret.add(handleSingleChar(s));
      }
    }

    return ret;
  }

  private static boolean isTokenChar(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '.');
  }

  private static void handlePreprocesor(State s) {
    while (s.hasContent()) {
      char ch = s.peekChar();
      if (ch == '\n') {
        break;
      }
      s.eatChar();
    }
  }

  private static CodeToken handleIdent(State s) {
    int fromPos = s.position();
    int fromLine = s.line();

    while (s.hasContent()) {
      char ch = s.peekChar();
      if (!isTokenChar(ch)) {
        break;
      }
      s.eatChar();
    }

    return s.createToken(CodeTokenType.Token, fromPos, fromLine);
  }

  private static CodeToken handleSingleChar(State s) {
    int fromPos = s.position();
    int fromLine = s.line();

    s.eatChar();

    return s.createToken(CodeTokenType.Char, fromPos, fromLine);
  }

  private static CodeToken handleString(State s) {
    char delim = s.peekChar();
    s.eatChar();

    int fromPos = s.position();
    int fromLine = s.line();

    while (s.hasContent()) {
      char ch = s.peekChar();

      if ( ch == '\\') {
        s.eatChar();
        s.eatChar(); // eat the escaped character
      }
      else if (ch == delim) {
        break;
      } else {
        s.eatChar();
      }
    }

    CodeToken ret = s.createToken(CodeTokenType.String, fromPos, fromLine);
    s.eatChar();
    return ret;
  }

  private static void handleComment(State s) {
    s.eatChar();

    if (s.peekChar() == '*') {
      s.eatChar();
      handleMultiLineComment(s);
    }
    else if (s.peekChar() == '/') {
      s.eatChar();
      handleSingleLineComment(s);
    }
  }

  private static void handleSingleLineComment(State s) {
    while (s.hasContent()) {
      char ch = s.peekChar();
      if (ch == '\n') {
        break;
      }
      s.eatChar();
    }
  }

  private static void handleMultiLineComment(State s) {
    while (s.hasContent()) {
      char ch = s.peekChar();
      if (ch == '*') {
        s.eatChar();

        ch = s.peekChar();
        if (ch == '/') {
          s.eatChar();
          break;
        }
      }
      s.eatChar();
    }
  }

  ///---

  private static class State
  {
    private char[] txt;
    private int pos;
    private int end;
    private int currentLine;

    public State(String txt) {
      this.txt = new char[txt.length()];
      this.pos = 0;
      this.end = txt.length();
      this.currentLine = 1;

      txt.getChars(0,txt.length(), this.txt, 0);
    }

    public boolean hasContent() {
      return pos < end;
    }

    public int position() {
      return pos;
    }

    public int line() {
      return currentLine;
    }

    public char peekChar() {
      return pos < end ? txt[pos] : 0;
    }

    public void eatChar() {
      char ch = txt[pos++];
      if (ch == '\n')
        currentLine++;
    }

    public CodeToken createToken(CodeTokenType type, int fromPos, int fromLine) {
      return new CodeToken(type, fromPos, pos - fromPos, txt, fromLine);
    }
  }

}
