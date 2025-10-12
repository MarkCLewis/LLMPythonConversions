import java.io.File

val lineRegex = """([A-Za-z0-9\.\-]+) ([A-Za-z0-9\._]+) ([A-Za-z0-9\._]+) ([A-Za-z0-9\._]+)|(real|user)\s+(\d+)m(\d+.\d+)s""".r

def stats(times: List[Double]): (Double, Double) = {
  val avg = times.sum / times.length
  val std = math.sqrt(times.map(t => (t - avg)*(t - avg)).sum / times.length)
  (avg, std)
}

def processTimeFile(file: File): Unit = {
  var benchmark = ""
  var version = ""
  var author = ""
  var language = ""
  var realSecs: List[Double] = Nil
  var userSecs: List[Double] = Nil
  val source = io.Source.fromFile(file)
  val lines = source.getLines()
  for (case lineRegex(bench, ver, auth, lang, ru, m, s) <- lines) {
    if (bench != null) {
      if (realSecs.nonEmpty) {
        val (ravg, rstd) = stats(realSecs)
        val (uavg, ustd) = stats(userSecs)
        println(f"$benchmark $version $author $language \\shortstack[c]{$$$ravg%1.2f \\pm $rstd%1.2f$$ \\\\ $$${uavg/ravg}%1.2f$$}")
        realSecs = Nil
        userSecs = Nil
      }
      benchmark = bench
      version = ver
      author = auth
      language = lang
    } else {
      if (ru == "real") {
        realSecs ::= 60*m.toDouble + s.toDouble
      } else {
        userSecs ::= 60*m.toDouble + s.toDouble
      }
    }
  }
  val (ravg, rstd) = stats(realSecs)
  val (uavg, ustd) = stats(userSecs)
  println(f"$benchmark $version $author $language \\shortstack[c]{$$$ravg%1.2f \\pm $rstd%1.2f$$ \\\\ $$${uavg/ravg}%1.2f$$}")
  source.close()
}

@main def processTimes(timeFileName: String): Unit = {
  processTimeFile(new File(timeFileName))
}
