name := "GHook"
organization := "Garret Leotta"
version := "1.0.0"

resolvers ++= Seq(
)

libraryDependencies ++= Seq(
  "org.scalafx" %% "scalafx" % "8.0.181-R13",
)

scalacOptions ++= Seq(
  "-unchecked",
  "-deprecation",
  "-feature",
)

mainClass in (Compile, run) := Some("ghook.GHookTest")
