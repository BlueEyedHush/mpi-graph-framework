package perftest

import org.apache.spark.{SparkConf, SparkContext}

object ClusterRunner {
  def main(args: Array[String]): Unit = {
    val sparkConf = new SparkConf().setAppName("example-graphx")
    implicit val sc = new SparkContext(sparkConf)

    val cliArgs = CliParser.parseCli(args.toList)

    Main.run(cliArgs.algorithm, cliArgs.iterations, cliArgs.graphPath)

    sc.stop()
  }
}
