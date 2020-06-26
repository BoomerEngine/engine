package generators;

import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

public enum SolutionType {
    FULL("full"),
    FINAL("final");

    //---

    public String name;
    public boolean buildAsLibs;
    public boolean includeTests;
    public boolean allowDevProjects;
    public boolean allowEngineProjects;

    //---

    SolutionType(String name_) {
        this.name = name_;

        this.buildAsLibs = name.equals("final");
        this.includeTests = name.equals("full");
        this.allowDevProjects = name.equals("full");
        this.allowEngineProjects = true;//name.equals("full");
    }

    public static Optional<SolutionType> mapName(String solutionName) {
        return Arrays.stream(values()).filter(str -> str.name.equals(solutionName)).findFirst();
    }

    static public String GetAllStrings() {
        return Arrays.stream(values()).map(val -> val.name).collect(Collectors.joining(","));
    }
}
