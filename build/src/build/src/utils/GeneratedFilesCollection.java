package utils;

import java.nio.file.Path;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;

public class GeneratedFilesCollection {
  public GeneratedFilesCollection() {
  }

  public GeneratedFile createFile(Path outputPath) {
    GeneratedFile g = new GeneratedFile(outputPath);
    synchronized(files) {
      files.add(g);
    }
    return g;
  }

  public int count() {
    synchronized(files) {
      return files.size();
    }
  }

  public int save(boolean touchExistingFile) {
    synchronized(files) {
      return files.stream().mapToInt(g -> g.save(touchExistingFile) ? 1 : 0).sum();
    }
  }

  //---

  private List<GeneratedFile> files = Collections.synchronizedList(new ArrayList<>());
}
