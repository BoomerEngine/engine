package utils;
import java.nio.file.Path;

public class CodeToken {
  private int pos;
  private int length;
  private int line;
  private char[] buffer;
  private CodeTokenType type;

  public Path path;

  public CodeToken(CodeTokenType type, int pos, int length, char[] buffer, int line) {
    this.type = type;
    this.pos = pos;
    this.length = length;
    this.line = line;
    this.buffer = buffer;
  }

  public CodeTokenType type() {
    return type;
  }

  public String toString() {
    return new String(buffer, pos, length);
  }

  public int line() {
    return line;
  }
}

