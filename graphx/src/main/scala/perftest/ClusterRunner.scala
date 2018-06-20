package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark.{SparkConf, SparkContext}

object ClusterRunner {
  def main(args: Array[String]): Unit = {
    Logger.getRootLogger.setLevel(Level.ERROR)

    val cliArgs = CliParser.parseCli(args.toList)

    val slurmJobId = sys.env.getOrElse("SLURM_JOB_ID", "unknownid")
    val slurmJobName = sys.env.getOrElse("SLURM_JOB_NAME", "unknownname")

    val sparkConf = new SparkConf().setAppName(s"${slurmJobId}_${slurmJobName}")
    Utils.configureKryo(sparkConf)

    implicit val sc = new SparkContext(sparkConf)

    Main.run(cliArgs.algorithm, cliArgs.iterations, cliArgs.graphPath)

    sc.stop()
  }
}
