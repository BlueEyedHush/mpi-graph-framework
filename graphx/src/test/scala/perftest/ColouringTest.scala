package perftest

import org.scalatest.FunSpec

class ColouringTest extends FunSpec {
  describe("Colouring") {
    describe("on powerlaw graph") {
      it("should not use same colour for neighbours") {
        TestUtils.withTestContext(implicit sc => {
          val g = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876")
          val r = Colouring.run(g)

          println(s"Colours: ${r.vertices.collect().map { case (vid, vdata) => s"($vid -> ${vdata.colour})"}.toSeq}")

            r.triplets
            .collect()
            .foreach(t =>
              assert(t.srcAttr.colour != t.dstAttr.colour, "(neighbouring vertices must be coloured differently)"))
        })
      }
    }
  }
}
