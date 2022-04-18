package jemalloctest

import scala.collection.mutable.ArrayBuffer
import scala.util.Random


class Funky(f: => Unit) extends Thread {
  val config = JemallocIntegration.mkConfig();

  def setSleep(s: Long) =
    JemallocIntegration.setSleep(config, s)

  override def run(): Unit = {
    println(s"Setting up for $config in ${Thread.currentThread()}")
    val ret = JemallocIntegration.setup(config);
    println(s"And we're off $ret")
    f
    JemallocIntegration.tearDown();
  }
}

object Funky {
  def apply(f: => Unit) = {
    new Funky(f);
  }
}

object Test extends App {
  val f = Funky {
    val vs = ArrayBuffer.empty[Long];
    println("Hello!")

    for(i <- 1 to 100) {
      val n = Random.nextLong(1000000)
      println(s"Allocating $n")
      vs += JemallocIntegration.alloc(n);
    }
    for(v <- vs) {
      JemallocIntegration.free(v)
    }

  }
  f.setSleep(1000)

  f.run()

}
