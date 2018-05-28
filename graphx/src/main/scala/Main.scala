import org.apache.spark._
import org.apache.spark.graphx._
import org.apache.spark.storage.StorageLevel
import org.apache.log4j.{Level, Logger}

import scala.io.Source

object Main {
  // Start Spark.
  def run(implicit sc: SparkContext) = {
    // Suppress unnecessary logging.
    Logger.getRootLogger.setLevel(Level.ERROR)

    def pwd = System.getProperty("user.dir")
    val path = s"$pwd/../graphs/data/powerlaw_25_2_05_876.el"

    println(s"\n### Loading edge list: ${path}\n")
    Source.fromFile(path).getLines().foreach(println)
    System.err.println("Edges loaded successfully")

    println("\n### Loading graph\n")
    // Load a graph.
    val g: Graph[Int, Int] = GraphLoader.edgeListFile(
      sc,
      path,
      edgeStorageLevel = StorageLevel.MEMORY_AND_DISK,
      vertexStorageLevel = StorageLevel.MEMORY_AND_DISK
    )

    // Calculate centralities.
    println("\n### Degree centrality\n")
    g.degrees.sortByKey().collect().foreach { case (n, v) => println(s"Node: ${n} -> Degree: ${v}") }

    println("\n### Betweenness centrality\n")
    val h: Graph[Double, Int] = Betweenness.run(g)
    h.vertices.sortByKey().collect().foreach { case (n, v) => println(s"Node: ${n} -> Betweenness: ${v}") }
  }
}
