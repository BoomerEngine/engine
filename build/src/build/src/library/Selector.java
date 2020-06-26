package library;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Selector {

  public static Selector createFromPattern(String platform, String config) {
    Selector ret = new Selector();
    ret.platforms = Arrays.asList(platform.split(";"));
    ret.configs = Arrays.asList(config.split(";"));
    return ret;
  }

  public boolean test(String platform, String config) {
    return matchList(platforms, platform) && matchList(configs, config);
  }

  //---

  private List<String> platforms; // x64, x86, Android, PS4
  private List<String> configs; // Debug, Release, Final

  private static boolean matchList(List<String> patterns, String test) {
    if (patterns.size() == 1 && patterns.get(0).equals("*")) {
      return true;
    }

    return patterns.stream().anyMatch(p -> p.equalsIgnoreCase(test));
  }

}
