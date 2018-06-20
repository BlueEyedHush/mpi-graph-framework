package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark.{SparkConf, SparkContext}

object ClusterRunner {
  def main(args: Array[String]): Unit = {

    val cliArgs = CliParser.parseCli(args.toList)

    if (!cliArgs.verbose)
      Logger.getRootLogger.setLevel(Level.ERROR)

    val slurmJobId = sys.env.getOrElse("SLURM_JOB_ID", "unknownid")
    val slurmJobName = sys.env.getOrElse("SLURM_JOB_NAME", "unknownname")

    val sparkConf = new SparkConf().setAppName(s"${slurmJobId}_${slurmJobName}")

    if (cliArgs.useKryo)
      Utils.configureKryo(sparkConf)

    implicit val sc = new SparkContext(sparkConf)

    val perExecMemStr = sc
      .getExecutorMemoryStatus
      .map { case (exec, (total, rem)) => s" $exec: $rem/$total" }
      .mkString("\n")
    println(s"memory available for caching per executor:\n$perExecMemStr\n")

    Main.run(cliArgs.algorithm, cliArgs.iterations, cliArgs.graphPath)

    sc.stop()
  }
}
