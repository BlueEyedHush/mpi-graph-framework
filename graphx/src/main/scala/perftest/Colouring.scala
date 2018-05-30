package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, VertexId}

import scala.collection.mutable.ListBuffer
import scala.reflect.ClassTag

/**
  * This implementation assumes, that we deal with undirected graph
  * Since in GraphX all edges are directed, each "undirected" edge must
  * be represented as two directed edges
  */
object Colouring {

  case class VertexData[VD](idAdvSent: Boolean = false,
                            iAmWaitingFor: Long = 0,
                            waitingForMe: Set[VertexId] = Set.empty,
                            coloursAlreadyUsedUp: Set[Int] = Set.empty,
                            colour: Option[Int] = None,
                            originalData: VD)

  trait Message
  case class NeighbourIdAdvertisment(id: Set[VertexId]) extends Message
  case class NeighbourChoseColour(colours: Set[Int]) extends Message
  case object Dummy extends Message

  def run[VD: ClassTag, ED: ClassTag](graph: Graph[VD, ED])(implicit sc: SparkContext): Graph[VertexData[VD], ED] = {
    graph
      .mapVertices((_, vdata) => VertexData(originalData = vdata))
      .pregel[Message](Dummy)((vid, vdata, msg) => {
        msg match {
          case NeighbourIdAdvertisment(idsSet) =>
            val (higherIdCount, lowerIdSet) = idsSet
              .map(nVid => if (nVid > vid) (1, Set.empty) else (0, Set(nVid)))
              .reduce { case ((c1, s1), (c2, s2)) => (c1 + c2, s1 ++ s2) }

            vdata.copy(
              iAmWaitingFor = vdata.iAmWaitingFor + higherIdCount,
              waitingForMe = vdata.waitingForMe ++ lowerIdSet
            )

          case NeighbourChoseColour(colourSet) =>
            val stillWaitingFor = vdata.iAmWaitingFor - colourSet.size

            val newChosenColour =
              if (stillWaitingFor == 0)
               Some(1)
              else
               None

            vdata.copy(
              coloursAlreadyUsedUp = vdata.coloursAlreadyUsedUp ++ colourSet,
              colour = newChosenColour
            )

          case Dummy => vdata
        }

      }, triplet => {
        triplet.srcAttr match {
          case VertexData(false, _, _, _, _, _) =>
            Iterator.single((triplet.dstId, NeighbourIdAdvertisment(Set(triplet.srcId))))
          case VertexData(true, _, _, _, Some(colour), _) =>
            Iterator.single((triplet.dstId, NeighbourChoseColour(Set(colour))))
          // todo: must put flag so that only one message is sent
          case _ =>
            Iterator.empty
        }
      }, {
        case (Dummy, Dummy) => Dummy
        case (NeighbourIdAdvertisment(s1), NeighbourIdAdvertisment(s2)) => NeighbourIdAdvertisment(s1 ++ s2)
        case (NeighbourChoseColour(s1), NeighbourChoseColour(s2)) => NeighbourChoseColour(s1 ++ s2)
        case (m1, m2) => throw new RuntimeException(s"received unexpected combination of messages:\n M1: $m1\n M2: $m2")
      })
  }

}
