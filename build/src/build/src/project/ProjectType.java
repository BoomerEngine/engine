package project;

import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

public enum ProjectType {
  SOURCES,
  RTTI_GENERATOR;

  ProjectType() {
  }
}
