package generators;

import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.Collection;

public enum PlatformType {
    WINDOWS("win", "windows,winapi"),
    UWP("uwp", "uwp,winapi"),
    IOS("ios", "ios,apple,mobile,posix,arm"),
    MAC("mac", "mac,apple,posix"),
    ANDROID("android", "linux,android,mobile,posix,arm"),
    LINUX("linux", "linux,posix");

    //---

    public String name;
    public List<String> filters;

    //---

    PlatformType(String name_, String filters_) {
        this.name = name_;
        this.filters = Arrays.asList(filters_.split(","));
    }

    public static Optional<generators.PlatformType> mapPlatform(String platformName) {
        return Arrays.stream(values()).filter(str -> str.name.equals(platformName)).findFirst();
    }

    public static List<PlatformType> mapFilter(String filter) {
        return Arrays.stream(values()).filter(x -> x.filters.stream().anyMatch(str -> str.equals(filter))).collect(Collectors.toList());
    }

    static public String GetAllStrings() {
        return Arrays.stream(values()).map(val -> val.name).collect(Collectors.joining(","));
    }
}
