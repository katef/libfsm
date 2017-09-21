LABELS=('A' .. 'Z').to_a + ('a'..'z').to_a

DEFAULT_NUM_NODES = 100
DEFAULT_AVG_CONN  = 5
DEFAULT_SEED      = 3147837
DEFAULT_MAX_END   = 5
DEFAULT_PROB_EPS = 0.1

$seed      = Integer(ARGV.shift || DEFAULT_SEED)
$num_nodes = Integer(ARGV.shift || DEFAULT_NUM_NODES)
$avg_conn  = Integer(ARGV.shift || DEFAULT_AVG_CONN)
$max_end   = Integer(ARGV.shift || DEFAULT_MAX_END)
$prob_eps  = Float(ARGV.shift || DEFAULT_PROB_EPS)
$num_edges = $num_nodes * $avg_conn

puts <<eos
# seed = #{$seed}
# num_nodes = #{$num_nodes}
# avg_conn  = #{$avg_conn}
# max_end   = #{$max_end}
# num_edges = #{$num_edges}
# prob_eps  = #{$prob_eps}

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
    lbl = ''
    if $prob_eps <= 0.0 || $rng.rand > $prob_eps
      lbl = LABELS[ $rng.rand(LABELS.size) ]
    end

    done = edges[ [src,dst] ].nil?
  end

  edges[ [src,dst] ] = lbl
  edge_list << [src,dst,lbl]
end

edge_list.sort!

edge_list.each do |src,dst,lbl|
  if lbl != "" then
    puts "#{src} -> #{dst} '#{lbl}';"
  else
    puts "#{src} -> #{dst};"
  end
end

puts
puts "start: #{start};"
puts "end:  #{ends.join(',')};" unless ends.empty?
puts

