package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, GraphLoader}
import org.apache.spark.storage.StorageLevel
import perftest.Main.Algorithm

object Utils {
  def stdGraphNameToPath(filename: String): String = {
    def pwd = System.getProperty("user.dir")
    s"$pwd/../graphs/data/$filename.elt"
  }

  def relGraphPathToPath(relPath: String): String = {
    def pwd = System.getProperty("user.dir")
    s"$pwd/$relPath"
  }

  def loadGraph(path: String)(implicit sc: SparkContext): Graph[Int, Int] = {
    println(s"Loading $path")
    GraphLoader.edgeListFile(
      sc,
      path,
      edgeStorageLevel = StorageLevel.MEMORY_AND_DISK,
      vertexStorageLevel = StorageLevel.MEMORY_AND_DISK
    )
  }
}

object CliParser {
  case class CliArguments(algorithm: Algorithm.Value = Algorithm.Bfs,
                          iterations: Int = 1,
                          graphPath: String = "../graphs/data/SimpleTestgraph.elt")

  def parseCli(args: List[String]): CliArguments = parseCliR(args, CliArguments())

  private def parseCliR(args: List[String], partiallyParsed: CliArguments): CliArguments = {
    args match {
      case Nil => partiallyParsed
      case "-g" :: graphPath :: tail =>
        parseCliR(tail, partiallyParsed.copy(graphPath = graphPath))
      case "-i" :: iterations :: tail =>
        parseCliR(tail, partiallyParsed.copy(iterations = iterations.toInt))
      case "-a" :: algorithm :: tail =>
        val a = algorithm.toLowerCase() match {
          case "bfs" => Algorithm.Bfs
          case "colouring" => Algorithm.Colouring
        }

        parseCliR(tail, partiallyParsed.copy(algorithm = a))
    }
  }
}
