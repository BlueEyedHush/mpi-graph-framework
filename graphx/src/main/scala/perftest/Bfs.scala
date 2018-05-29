package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, VertexId}

import scala.collection.mutable.ListBuffer
import scala.reflect.ClassTag

object Bfs {
  def run[ED: ClassTag](graph: Graph[Boolean, ED], start: VertexId)(implicit sc: SparkContext): Graph[Boolean, ED] = {
    val visited = ListBuffer[Long]()
    val onVisit: Long => Unit =
      vid => println(s"visisted $vid")

    graph
      .pregel(false)((vid, vdata, msg) => {
        // initial iteration
        if (!msg)
          if (vid == start) {
            onVisit(vid)
            true
          } else false
        // any other iteration, if we received any message it means we're visited
        else {
          onVisit(vid)
          true
        }
      }, triplet => {
        if (triplet.srcAttr && !triplet.dstAttr) Iterator.single((triplet.dstId, true))
        else Iterator.empty
      }, (m1, m2) =>
        // only messages in circulation are 'visited' messages, we could OR them, but this is simple
        true
      )
  }
}
