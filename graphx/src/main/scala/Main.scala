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

    println("\n### Loading graph\n")
    // Load a graph.
    val path = "src/main/resources/edge_list_1.txt"

    println(s"\n### Loading edge list: ${path}\n")
    Source.fromFile(path).getLines().foreach(println)

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
