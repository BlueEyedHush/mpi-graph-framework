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
    val path = s"$pwd/../graphs/data/powerlaw_25_2_05_876.elt"

    println(s"\n### Loading edge list: ${path}\n")
    Source.fromFile(path).getLines().foreach(println)
    System.err.println("Edges loaded successfully")

    println("\n### Loading graph\n")
    // Load a graph.

    val g: Graph[Boolean, Int] = GraphLoader.edgeListFile(
      sc,
      path,
      edgeStorageLevel = StorageLevel.MEMORY_AND_DISK,
      vertexStorageLevel = StorageLevel.MEMORY_AND_DISK
    ).mapVertices((vid, _) => false)

    // Calculate centralities.
    println("\n### Bfs\n")
    val (startVertexId, _) = g.vertices.take(1)(0)
    val visisted = Bfs.run(g, startVertexId)
    require(visisted.size == g.numVertices, "all vertices must be visisted")
    require(visisted.toSet.size == visisted.size, "all recorded visits must be unique")
  }
}
