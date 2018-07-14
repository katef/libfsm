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

[ -z "${GENGRAPH}" ] && GENGRAPH="`dirname $0`/gengraph"
[ -z "${FSM}" ] && FSM=./build/bin/fsm

if [ ! -x "${FSM}" ]; then
  echo "Cannot find fsm tool at $FSM"
  echo "Either set FSM variable or run from top level of source"
  exit 1
fi

if [ ! -f "${GENGRAPH}" ]; then
  echo "Cannot find the gengraph script, expected at: $GENGRAPH"
  echo "Please set the GENGRAPH variable"
  exit 1
fi

[ -z "${BUILD}" ] && BUILD="./build"
if [ -z "${OUTDIR}" ]; then
  echo "OUTDIR is unset, trying to set from BUILD directory ${BUILD}"
  if [ ! -d "${BUILD}" ]; then
    echo "OUTDIR is not set and cannot find BUILD directory ${BUILD}."
    echo "Either set OUTDIR or BUILD or run from the top level of source"
    exit 1
  fi

  OUTDIR="${BUILD}/fuzz/testintsub"
fi

echo "Output to ${OUTDIR}"
mkdir -p ${OUTDIR}

i=0
while [ ${i} -lt ${NSEED} ]; do
  s=$(( ${SEED0} + ${i} ))

  a="${OUTDIR}/test-a-${s}.fsm"
  b="${OUTDIR}/test-b-${s}.fsm"
  ccuc="${OUTDIR}/intersect-ccuc-${s}.fsm"
  out="${OUTDIR}/intersect-${s}.fsm"

  cuc="${OUTDIR}/subtract-ccuc-${s}.fsm"
  sout="${OUTDIR}/subtract-${s}.fsm"

  case ${SUITE} in
    large)
      # graphs are large and difficult to check by hand, though not bad for
      # testing
      awk -f $GENGRAPH ${s} 10 4 3 > $a
      awk -f $GENGRAPH ${s} 10 8 3 > $b
      ;;

    medium)
      # still too large to easily check by hand
      awk -f $GENGRAPH ${s} 8 3 3 'abcdef' > $a
      awk -f $GENGRAPH ${s} 6 3 4 'abcdef' > $b
      ;;

    small|*)
      # these are sometimes large and sometimes a good size.
      # the NFAs are small, but can result in large DFAs.
      #
      # uses a reduced alphabet (abcd)
      awk -f $GENGRAPH ${s} 4 3 2 'abcd' > $a
      awk -f $GENGRAPH ${s} 4 3 1 'abcd' > $b
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

