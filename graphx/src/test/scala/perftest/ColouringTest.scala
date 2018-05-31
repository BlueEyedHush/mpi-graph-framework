package perftest

import org.scalatest.FunSpec

class ColouringTest extends FunSpec {
  describe("Colouring") {
    describe("on powerlaw graph") {
      it("should not use same colour for neighbours") {
        TestUtils.withTestContext(implicit sc => {
          val g = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876")
          Colouring.run(g)
            .triplets
            .collect()
            .foreach(t =>
              assert(t.srcAttr.colour != t.dstAttr.colour, "neighbouring vertices must be coloured differently"))
        })
      }
    }
  }
}
