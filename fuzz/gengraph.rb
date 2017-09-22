DEFAULT_LABELS=('A' .. 'Z').to_a + ('a'..'z').to_a

DEFAULT_NUM_NODES = 100
DEFAULT_AVG_CONN  = 5
DEFAULT_SEED      = 3147837
DEFAULT_MAX_END   = 5

$seed      = Integer(ARGV.shift || DEFAULT_SEED)
$num_nodes = Integer(ARGV.shift || DEFAULT_NUM_NODES)
$avg_conn  = Integer(ARGV.shift || DEFAULT_AVG_CONN)
$max_end   = Integer(ARGV.shift || DEFAULT_MAX_END)
$labels    = ARGV.shift || DEFAULT_LABELS
$num_edges = $num_nodes * $avg_conn

# XXX - add sanity check of number of labels, avg_conn, and num_nodes

puts <<eos
# seed = #{$seed}
# num_nodes = #{$num_nodes}
# avg_conn  = #{$avg_conn}
# max_end   = #{$max_end}
# num_edges = #{$num_edges}

eos

$rng = Random.new($seed)

start = $rng.rand($num_nodes)
ends  = (1..$num_nodes).to_a.shuffle(random: $rng).take(1+$rng.rand($max_end)).sort

puts <<EOS
# seed      = #{$seed}
# num_nodes = #{$num_nodes}
# avg_conn  = #{$avg_conn}
# num_edges = #{$num_edges}
# max_end   = #{$max_end}
# ends: #{ends.join(" ")}
EOS

edges = {}
edge_list = []
$num_edges.times do
  done = false
  while !done do
    src = $rng.rand($num_nodes)
    dst = $rng.rand($num_nodes)
    lbl = $labels[ $rng.rand($labels.size) ]

    done = edges[ [src,dst] ].nil?
  end

  edges[ [src,dst] ] = lbl
  edge_list << [src,dst,lbl]
end

edge_list.sort!

edge_list.each do |src,dst,lbl|
  puts "#{src} -> #{dst} '#{lbl}';"
end

puts
puts "start: #{start};"
puts "end:  #{ends.join(',')};" unless ends.empty?
puts

