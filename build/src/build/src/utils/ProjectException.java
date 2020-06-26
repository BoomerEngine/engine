package utils;

import java.nio.file.Path;

/**
 * Created by RexDex on 2/27/2017.
 */
public class ProjectException extends RuntimeException
{
  public ProjectException(String desc)
  {
    super(desc, null);
    this.fileLine = -1;
  }

  public ProjectException(String desc, Throwable cause)
  {
    super(desc, cause);
    this.fileLine = -1;
  }

  public ProjectException location(Path path, int line)
  {
    this.filePath = path;
    this.fileLine = line;
    return this;
  }

  public ProjectException location(Path path)
  {
    this.filePath = path;
    return this;
  }

  public ProjectException location(int line)
  {
    this.fileLine = line;
    return this;
  }

  @Override
  public String toString()
  {
    String ret = "";

    if (filePath != null) {
      ret += filePath.toString();

      if (fileLine != -1) {
        ret += String.format("(%d)", fileLine);
      }

      ret += ": error: ";
    }

    return ret + super.toString();
  }

  //--

  private Path filePath;
  private int fileLine;
}
