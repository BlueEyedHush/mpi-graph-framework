name := "graphx-perf-comp"

version := "1.0"

scalaVersion := "2.11.8"

libraryDependencies ++= Seq(
  ("org.apache.spark" %% "spark-core" % "2.0.2" excludeAll("org.scalatest" %% "scalatest")) % Provided,
  "org.apache.spark" %% "spark-graphx" % "2.0.2" % Provided,
  "org.scalatest" %% "scalatest" % "3.0.5" % Test
)

fork in run := true
parallelExecution in Test := false // each test creates its own SparkContext and they conflict during parallel exec
test in assembly := {} // don't run tests when building fat jar