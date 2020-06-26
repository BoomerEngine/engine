package utils;

/**
 * Created by RexDex on 2/27/2017.
 */
public class Tokenizer {

  //---

  // initialize from string
  public Tokenizer(String textToTokenize) {
    this.txt = new char[textToTokenize.length()];
    this.pos = 0;
    this.end = textToTokenize.length();
    this.currentLine = 1;

    textToTokenize.getChars(0,textToTokenize.length(), txt, 0);
  }

  // do we have anything more to parse ?
  public boolean hasContent() {
    return shipWhitespaces();
  }

  // get the current line number we are at
  public int getLineNumber() {
    return currentLine;
  }

  // scans until a given token is found
  public boolean findKeyword(String pattern) {
    int len = pattern.length();
    if (pos + len > end) // string is to long to be matched withing the remaining portion of the buffer
      return false;

    int maxSearch = end - len;
    while (pos < maxSearch) {
      if (matchSubText(pos, pattern)) {
        pos += len; // skip the matched pattern
        return true;
      }

      pos += 1; // TODO: optimize ;]
    }

    return false;
  }

  // returns true if a keyword is found, advances the parsing position if it's foundf
  public boolean parseKeyword(String pattern) {
    // skip empty content, detect EOF
    if (!shipWhitespaces())
      return false;

    int len = pattern.length();
    if (pos + len > end) // string is to long to be matched withing the remaining portion of the buffer
      return false;

    // test all chars, faster than creating sub-string
    if (!matchSubText(pos, pattern))
      return false;

    // matches
    pos += len;
    return true;
  }

  // extract a single text token
  // if the token is in quotes it's extracted fully to the closing quote
  // if the token is not in quotes it's treated as identifier (letters, number and and underscore '_')s
  public String parseToken() {
    // skip empty content, detect EOF
    if (!shipWhitespaces())
      return "";

    String ret = "";

    // quotes or normal token ?
    if (txt[pos] == '\"')
    {
      ++pos; // skip the quote symbol

      while (pos < end && txt[pos] != '\"') {
        char ch = eatChar();
        ret += String.valueOf(ch);
      }

      if (pos < end) {
        ++pos; // skip the end quote symbol
      }
    } else {
      while (pos < end && isValidIdentChar(txt[pos])) {
        char ch = eatChar();
        ret += String.valueOf(ch);
      }
    }

    return ret;
  }

  // extract a single delimited text token
  // if the token is in quotes it's extracted fully to the closing quote
  // if the token is not in quotes it extracted until one of the delimiting chars if found
  public String parseDelimitedToken(String limitChars) {
    // skip empty content, detect EOF
    if (!shipWhitespaces())
      return "";

    String ret = "";

    // quotes or normal token ?
    if (txt[pos] == '\"')
    {
      ++pos; // skip the quote symbol

      while (pos < end && txt[pos] != '\"') {
        char ch = eatChar();
        ret += String.valueOf(ch);
      }

      if (pos < end) {
        ++pos; // skip the end quote symbol
      }
    } else {
      while (pos < end && !isCharFromSet(txt[pos], limitChars)) {
        char ch = eatChar();
        ret += String.valueOf(ch);
      }
    }

    return ret;
  }

  //---

  private char eatChar() {
    return txt[pos++];
  }

  private static boolean isValidIdentChar(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch == '_') || (ch >= '0' && ch <= '9') || (ch == '(' || ch == ')');
  }

  private static boolean isCharFromSet(char ch, String str) {
    return str.indexOf(ch) > -1;
  }

  private boolean matchSubText(int atPos, String pattern)
  {
    int len = pattern.length();
    for (int i=0; i<len; ++i) {
      if (txt[atPos+i] != pattern.charAt(i)) {
        return false;
      }
    }

    return true;
  }

  private boolean shipWhitespaces() {
    while (pos < end && txt[pos] <= 32) {

      if (txt[pos] == '\n')
        currentLine += 1;

      ++pos;
    }

    return pos < end;
  }

  private char[] txt;
  private int pos;
  private int end;
  private int currentLine;
}
