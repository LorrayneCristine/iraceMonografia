#!/bin/bash

# Verifica se foi passado o tipo de movimento
if [ -z "$1" ]; then
  echo "Uso: $0 <MOV_TYPE>"
  exit 1
fi

MOV_TYPE=$1

# Caminho para o executável
BIN="./binTIP"
OUTFILE="resultados_MOV${MOV_TYPE}.csv"

# Parâmetros da elite (adicione MOV_TYPE)
PARAMS="--MOV_TYPE $MOV_TYPE --TEMP_INIT 0.03 --TEMP_FIM 1.0 --MCL 500 --TEMP_DIST 4 --TYPE_UPDATE 1 --TEMP_UPDATE 5"

# Cabeçalho, só adiciona se o arquivo não existe
if [ ! -f "$OUTFILE" ]; then
  echo "Instância,MelhorCusto,MelhorTempo,MediaCusto,MediaTempo,Slots,Ferramentas,Magazine,Parametros" > "$OUTFILE"
fi

# Para cada instância na pasta
for INSTANCE in instances/*.txt; do
  INST=$(basename "$INSTANCE")
  echo "Rodando $INST..."

  CUSTOS=()
  TEMPOS=()
  MAGAZINE=""
  SLOTS=""
  FERRAMENTAS=""

  for run in {1..10}; do
    OUTPUT=$($BIN "$INSTANCE" $PARAMS)

    CUSTO=$(echo "$OUTPUT" | grep -i "Custo" | grep -Eo '[0-9]+' | head -n1)
    TEMPO=$(echo "$OUTPUT" | grep -i "Tempo" | grep -Eo '[0-9.]+')
    SLOTS=$(echo "$OUTPUT" | grep -i "slots" | grep -Eo '[0-9]+' | head -n 1)
    FERRAMENTAS=$(echo "$OUTPUT" | grep -i "ferramenta" | grep -Eo '[0-9]+' | head -n 1)
    MAGAZINE=$(echo "$OUTPUT" | grep -i "magazine" | cut -d ':' -f2- | tr -d '[]' | tr '\n' ' ')

    CUSTOS+=("$CUSTO")
    TEMPOS+=("$TEMPO")
  done

  # Encontra o índice do menor custo
  minIndex=0
  minCusto=${CUSTOS[0]}
  for i in "${!CUSTOS[@]}"; do
    if (( ${CUSTOS[i]} < minCusto )); then
      minCusto=${CUSTOS[i]}
      minIndex=$i
    fi
  done

  # Calcula média das outras 9 execuções
  sumCusto=0
  sumTempo=0
  for i in "${!CUSTOS[@]}"; do
    if [ "$i" -ne "$minIndex" ]; then
      sumCusto=$((sumCusto + CUSTOS[i]))
      sumTempo=$(echo "$sumTempo + ${TEMPOS[i]}" | bc)
    fi
  done

  mediaCusto=$((sumCusto / 9))
  mediaTempo=$(echo "scale=6; $sumTempo / 9" | bc)

  # Salva resultado
  echo "$INST,$minCusto,${TEMPOS[$minIndex]},$mediaCusto,$mediaTempo,$SLOTS,$FERRAMENTAS,\"$MAGAZINE\",\"$PARAMS\"" >> "$OUTFILE"
done

