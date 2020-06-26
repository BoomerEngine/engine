package project;

import utils.ProjectException;

import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.CRC32;

public class Group {

    public final Solution solution;
    public final Group parentGroup;
    public final String shortName;
    public final String mergedName;

    public List<Project> projects = new ArrayList<>();
    public List<Group> childGroups = new ArrayList<>();;
    //----

    public Group(Solution sol, Group parent, String name) {
      this.solution = sol;
      this.parentGroup = parent;
      this.shortName = name;
      this.projects = new ArrayList<>();
      this.childGroups = new ArrayList<>();
      this.mergedName = (parent != null) ? (parent.mergedName + "_" + name) : name;
    }

    //--

    public Group createChildGroup(String name) {
      for (Group g : childGroups)
        if (g.shortName == name)
          return g;

      Group g = new Group(solution, this, name);
      childGroups.add(g);
      return g;
    }

    public Project createSourcesProject(Path projectPath, Module module) {
      Project p = new ProjectSources(solution, this, projectPath, module);
      projects.add(p);
      return p;
    }

    //----

    public void collectAllProjects(List<Project> projectList) {
        for (Project p : projects)
            projectList.add(p);

        for (Group g : childGroups)
            g.collectAllProjects(projectList);
    }

    public String getPersistentGuid() {
        return utils.Utils.guidFromText("group_" + mergedName);
    }

    public void makeDeterministic() {
        childGroups.sort((a,b) -> a.shortName.compareTo(b.shortName));
        projects.sort((a,b) -> a.shortName.compareTo(b.shortName));
        childGroups.forEach(g -> g.makeDeterministic());
    }

    //----

}
