package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark._
import org.apache.spark.graphx._

object Main {

  object Algorithm extends Enumeration {
    type Algorithm = Value
    val Bfs, Colouring = Value
  }

  def printTime(name: String, time: Long) = println(s"$name: $time (${time/1000000000.0})")

  // Start Spark.
  def run(algo: Algorithm.Value, iterationCount: Int, relativeGraphPath: String)(implicit sc: SparkContext) = {
    // Suppress unnecessary logging
    Logger.getRootLogger.setLevel(Level.ERROR)

    println(s"graph: $relativeGraphPath algorithm: $algo iterations: $iterationCount")

    for (i <- 0 until iterationCount) {
      val graphLoadingStart = System.nanoTime()
      val g: Graph[Int, Int] = Utils.loadGraph(Utils.relGraphPathToPath(relativeGraphPath))
      val graphLoadingTime = System.nanoTime() - graphLoadingStart
      printTime("graph", graphLoadingTime)

      val algoExecutionTime = algo match {
        case Algorithm.Bfs =>
          val bg = g.mapVertices((vid, _) => false)
          val (startVertexId, _) = g.vertices.take(1)(0)

          val start = System.nanoTime()
          Bfs.run(bg, startVertexId)
          System.nanoTime() - start

        case Algorithm.Colouring =>
          val start = System.nanoTime()
          Colouring.run(g)
          System.nanoTime() - start
      }

      printTime("algo", algoExecutionTime)
    }
  }
}
