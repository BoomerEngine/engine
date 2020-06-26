package utils;

import java.util.Iterator;
import java.util.List;

public class CodeTokenStream {
  public CodeTokenStream(List<CodeToken> tokens) {
    this.tokens = tokens.toArray(new CodeToken[0]);
    this.pos = 0;
    this.end = tokens.size() - 1;
  }

  public boolean hasContent() {
    return pos <= end;
  }

  public String peekText(int distance) {
    if (pos + distance >= end)
      return "";

    return tokens[pos + distance].toString();
  }

  public CodeToken eat() {
    if (pos > end)
      throw new ProjectException("Unexpected end of file").location(tokens[end].path, tokens[end].line());

    prev = tokens[pos++];
    return prev;
  }

  public void consume(int count) {
    pos += count;
  }

  public CodeToken consumed() {
    return prev;
  }

  //--

  private CodeToken[] tokens;
  private CodeToken prev;
  private int pos;
  private int end;
}
