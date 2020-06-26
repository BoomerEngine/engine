package generators;

import project.Solution;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;

public interface Generator {
  void generateContent(Solution sol, KeyValueTable params, GeneratedFilesCollection files);
}
