package utils;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Path;
import java.nio.file.Files;
import java.nio.file.attribute.FileTime;

public class GeneratedFile {

  public GeneratedFile(Path outputPath) {
    this.outputPath = outputPath;
  }

  public static boolean ShouldCopyFile(Path from, Path to) {
    if (!Files.exists(to))
      return true;

    try {
      FileTime fromTime = Files.getLastModifiedTime(from);
      FileTime toTime = Files.getLastModifiedTime(to);
      return fromTime.compareTo(toTime) != 0;
    }
    catch (IOException e) {
      return true;
    }
  }

  public static boolean ShouldGenerateFile(Path from, Path to) {
    if (!Files.exists(to))
      return true;

    try {
      FileTime fromTime = Files.getLastModifiedTime(from);
      FileTime toTime = Files.getLastModifiedTime(to);
      //System.out.println(String.format("LMD of %s: %s", from, fromTime.toString()));
      //System.out.println(String.format("LMD of %s: %s", to, toTime.toString()));
      return fromTime.compareTo(toTime) > 0;
    }
    catch (IOException e) {
      return true;
    }
  }

  public void writeln(String str) {
    builder.append(str);
    builder.append("\n");
  }

  public void writelnf(String format, Object... args) {
    writeln(String.format(format, args));
  }

  public boolean save(boolean touchExistingFile) {
    // flush builder to get content to save
    String content = builder.toString();

    // load existing content and it's the same as the shit we want to save we can skip saving
    try {
      String existingContent = utils.Utils.loadFileToString(outputPath, Charset.defaultCharset());
      if (existingContent.equals(content)) {
        if (touchExistingFile)
          utils.Utils.touchFile(outputPath);
        return false;
      } else {
        System.out.printf("Content of file '%s' needs to be refreshed\n", outputPath);
      }
    } catch (IOException e) {
      // the IOException is allowed here
    }

    // save new content
    try {
      utils.Utils.saveStringToFile(outputPath, content, Charset.defaultCharset());
      return true;
    } catch (IOException e) {
      throw new ProjectException("Failed to save file", e).location(outputPath);
    }
  }

  //---

  private Path outputPath;
  private StringBuilder builder = new StringBuilder();
}
