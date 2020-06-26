package project;

import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

public enum FileType {
  SOURCE("cpp,c,cxx"),
  HEADER("h,hpp,hxx,inl"),
  ASSEMBLY("asm,s"),
  RESOURCE("rc"),
  FLEX("flex"),
  BISON("bison"),
  ANTLR("g4"),
  CONFIG("xml"),
  NATVIS("natvis"),
  VSIXMANIFEST("vsixmanifest"),
  PROTO("proto"),
  QTRESOURCES("qrc"),
  QTUI("ui");

  //---

  FileType(String extensions) {
    this.extensions = Arrays.asList(extensions.split(","));
    this.matcher = FileSystems.getDefault().getPathMatcher(getMatchPatternString());
  }

  /*
   * get the match pattern for the file system path matcher
   */
  public String getMatchPatternString() {
    return "glob:*.{" + extensions.stream().collect(Collectors.joining(",")) + "}";
  }

  /*
   * get the file type that matches the given path best
   * this checks the file extension
   */
  public static Optional<FileType> mapExtension(Path path) {
    return Arrays.stream(values()).filter(x -> x.matcher.matches(path.getFileName())).findFirst();
  }

  public List<String> getExtensions() {
    return extensions;
  }

  //---


  // file extensions supported by this file type
  private List<String> extensions;

  // path matcher to check the extensions
  private PathMatcher matcher;
}
