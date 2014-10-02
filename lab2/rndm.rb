require 'benchmark'
t = Benchmark.measure {
  f = File.open("netdir/k1000")
  f.seek(12, IO::SEEK_SET)
  puts f.read(10)
  f.seek(123, IO::SEEK_SET)
  puts f.read(10)
  f.seek(321, IO::SEEK_SET)
  puts f.read(10)
  f.seek(321, IO::SEEK_SET)
  puts f.read(10)
  f.seek(322123, IO::SEEK_SET)
  puts f.read(10)
  f.seek(12, IO::SEEK_SET)
  puts f.read(10)
  f.seek(1, IO::SEEK_SET)
  puts f.read(10)
  f.seek(213, IO::SEEK_SET)
  puts f.read(10)
  f.seek(3, IO::SEEK_SET)
  puts f.read(10)
  f.seek(10000, IO::SEEK_SET)
  puts f.read(10)
  f.close 
}

puts t
