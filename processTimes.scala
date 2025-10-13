import java.io.File
import scala.collection.mutable

val lineRegex = """([A-Za-z0-9\.\-]+) ([A-Za-z0-9\._]+) ([A-Za-z0-9\._]+) ([A-Za-z0-9\._]+)|(real|user)\s+(\d+)m(\d+.\d+)s""".r

val benchmarks = List("k-nuclcotide", "spectral-norm",
  "binary-trees",   "mandelbrot",
  "fannkuch-redux", "nbody",        "regex-redux",
  "fasta",          "pidigits",     "reverse-complement",
  "photometry",     "kd-tree"
).sorted

val authors = List("Human", "Gemini2.5", "ChatGPT", "Claude")

val languages = List("python", "c", "rust")

def stats(times: List[Double]): (Double, Double) = {
  val avg = times.sum / times.length
  val std = math.sqrt(times.map(t => (t - avg)*(t - avg)).sum / times.length)
  (avg, std)
}

def processTimeFile(file: File): Map[(String, String, String, String), String] = {
  var benchmark = ""
  var version = ""
  var author = ""
  var language = ""
  var realSecs: List[Double] = Nil
  var userSecs: List[Double] = Nil
  val source = io.Source.fromFile(file)
  val lines = source.getLines()
  val values = mutable.Buffer[((String, String, String, String), String)]()
  for (case lineRegex(bench, ver, auth, lang, ru, m, s) <- lines) {
    if (bench != null) {
      if (realSecs.nonEmpty) {
        val (ravg, rstd) = stats(realSecs)
        val (uavg, ustd) = stats(userSecs)
        val tableEntry = f"$$$ravg%1.2f \\pm $rstd%1.2f$$"
        println(f"$benchmark $version $author $language $tableEntry")
        values += (((benchmark, language, version, author), tableEntry))
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
  val tableEntry = f"$$$ravg%1.2f \\pm $rstd%1.2f$$"
  println(f"$benchmark $version $author $language $tableEntry")
  values += (((benchmark, language, version, author), tableEntry))
  source.close()
  values.groupBy(_(0)).map { case (key, lst) =>
    key -> lst.head(1)
  }
}

@main def processTimes(timeFileNames: String*): Unit = {
  val data = timeFileNames.foldLeft(Map[(String, String, String, String), String]())((m, timeFileName) => m ++ processTimeFile(new File(timeFileName)))
  println(data)
  for bench <- benchmarks do {
    for i <- 1 to 8 do {
      val version = f"Version$i"
      for language <- languages do {
        val values = authors.map { case author =>
          val key = (bench, language, version, author)
          data.get(key).getOrElse("")
        }
        if values.exists(_.nonEmpty) then println(f"$bench, $language, $version, ${values.mkString(" & ")}")
      }
    }
  }
}
