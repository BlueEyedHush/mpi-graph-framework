package perftest

import org.scalatest.FunSpec
import perftest.CliParser.CliArguments
import perftest.Main.Algorithm

class CliParserTest extends FunSpec {
  describe("CliParser") {
    it("should correclty parse '-i 10'") {
      val cliArgs = CliParser.parseCli(List("-i", "10"))
      assert(cliArgs === CliArguments(iterations = 10))
    }

    val gname = "../graph/data/SimpleTestGraph.elt"
    it(s"should correclty parse '-g <gname>'") {
      val cliArgs = CliParser.parseCli(List("-g", gname))
      assert(cliArgs === CliArguments(graphPath = gname))
    }

    describe("should correctly parse algorithm") {
      it("bfs") {
        val cliArgs = CliParser.parseCli(List("-a", "Bfs"))
        assert(cliArgs === CliArguments(algorithm = Algorithm.Bfs))
      }

      it("colouring") {
        val cliArgs = CliParser.parseCli(List("-a", "Colouring"))
        assert(cliArgs === CliArguments(algorithm = Algorithm.Colouring))
      }
    }

    describe("should correctly parse when all options present") {
      val cliArgs = CliParser.parseCli(List("-a", "Bfs", "-i", "10", "-g", "graphPath"))
      assert(cliArgs === CliArguments(algorithm = Algorithm.Bfs, iterations = 10, graphPath = "graphPath"))
    }
  }
}
