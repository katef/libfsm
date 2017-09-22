#! /bin/bash

# need bash for <(...) constructs

SEED0=$1 ; shift
NSEED=$1 ; shift
SUITE=$1 ; shift

usage() {
  echo "use: $0 <seed> [<n>] [suite]"
  echo "  <seed>   initial random number seed"
  echo "  <n>      number of graphs to generate"
  echo "           each graph generated increments the seed"
  echo "  <suite>  size of graphs to test (large|medium|small)"

  exit 1
}

[ -z "${SEED0}" ] && usage
[ -z "${NSEED}" ] && NSEED=100
[ -z "${SUITE}" ] && SUITE=small

[ -z "${FSM}" ] && FSM=./build/bin/fsm
[ -z "${FUZZ}" ] && FUZZ="fuzz.sfs"

mkdir -p ${FUZZ}

i=0
while [ ${i} -lt ${NSEED} ]; do
  s=$(( ${SEED0} + ${i} ))

  a="${FUZZ}/test-a-${s}.fsm"
  b="${FUZZ}/test-b-${s}.fsm"
  ccuc="${FUZZ}/intersect-ccuc-${s}.fsm"
  out="${FUZZ}/intersect-${s}.fsm"

  cuc="${FUZZ}/subtract-ccuc-${s}.fsm"
  sout="${FUZZ}/subtract-${s}.fsm"

  case ${SUITE} in
    large)
      # graphs are large and difficult to check by hand, though not bad for
      # testing
      ruby gengraph.rb ${s} 10 4 3 > $a
      ruby gengraph.rb ${s} 10 8 3 > $b
      ;;

    medium)
      # still too large to easily check by hand
      ruby gengraph.rb ${s} 8 3 3 'abcdef' > $a
      ruby gengraph.rb ${s} 6 3 4 'abcdef' > $b
      ;;

    small|*)
      # these are sometimes large and sometimes a good size.
      # the NFAs are small, but can result in large DFAs.
      #
      # uses a reduced alphabet (abcd)
      ruby gengraph.rb ${s} 4 3 2 'abcd' > $a
      ruby gengraph.rb ${s} 4 3 1 'abcd' > $b
      ;;
  esac

  # test intersection
  ${FSM} -pt union \
      <(${FSM} -pt complement $a) \
      <(${FSM} -pt complement $b) | \
    ${FSM} -pt complement | ./build/bin/fsm -pm > ${ccuc}

  ${FSM} -pt intersect $a $b | ./build/bin/fsm -pm > ${out}

  echo -ne "test ${s}\t\t"
  ${FSM} -t equal ${out} ${ccuc}
  if [ $? -eq 0 ]; then
    echo -ne "[PASSED]\t"
  else
    echo -ne "[FAILED]\t"
    ${FSM} -pl dot $a | dot -Tpng > ${a%.fsm}.png
    ${FSM} -pl dot $b | dot -Tpng > ${b%.fsm}.png
    ${FSM} -pl dot ${ccuc} | dot -Tpng > ${ccuc%.fsm}.png
    ${FSM} -pl dot ${out}  | dot -Tpng > ${out%.fsm}.png
  fi

  # test subtraction
  ${FSM} -pt union <(${FSM} -pt complement $a) $b | \
    ${FSM} -pt complement | ./build/bin/fsm -pm > ${cuc}

  ${FSM} -pt subtract $a $b | ./build/bin/fsm -pm > ${sout}

  ${FSM} -t equal ${sout} ${cuc}
  if [ $? -eq 0 ]; then
    echo -e "[PASSED]"
  else
    echo -e "[FAILED]"
    ${FSM} -pl dot $a | dot -Tpng > ${a%.fsm}.png
    ${FSM} -pl dot $b | dot -Tpng > ${b%.fsm}.png
    ${FSM} -pdl dot $a | dot -Tpng > ${a%.fsm}_d.png
    ${FSM} -pdl dot $b | dot -Tpng > ${b%.fsm}_d.png
    ${FSM} -pl dot ${cuc} | dot -Tpng > ${cuc%.fsm}.png
    ${FSM} -pl dot ${sout}  | dot -Tpng > ${sout%.fsm}.png
  fi

  i=$(( $i + 1 ))
done

