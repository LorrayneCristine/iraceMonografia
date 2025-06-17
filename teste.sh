#!/bin/bash

# Caminho para o executável
BIN="./binTIP"
OUTFILE="resultados.csv"

# Parâmetros da elite
PARAMS="--TEMP_INIT 0.03 --TEMP_FIM 1.0 --MCL 500 --TEMP_DIST 4 --TYPE_UPDATE 1 --TEMP_UPDATE 5"

# Cabeçalho, só adiciona se o arquivo não existe
if [ ! -f "$OUTFILE" ]; then
  echo "Execução,Instância,Custo,Tempo,Slots,Ferramentas,Magazine,Parametros" > "$OUTFILE"
fi

# Para cada instância na pasta
for INSTANCE in instances/*.txt; do
  INST=$(basename "$INSTANCE")
  echo "Rodando $INST..."

  for run in {1..5}; do
    OUTPUT=$($BIN "$INSTANCE" $PARAMS)

    # Extrações com grep/sed
    CUSTO=$(echo "$OUTPUT" | grep -i "Custo" | grep -Eo '[0-9]+')
    TEMPO=$(echo "$OUTPUT" | grep -i "Tempo" | grep -Eo '[0-9.]+')
    SLOTS=$(echo "$OUTPUT" | grep -i "slots" | grep -Eo '[0-9]+' | head -n 1)
    FERRAMENTAS=$(echo "$OUTPUT" | grep -i "ferramenta" | grep -Eo '[0-9]+' | head -n 1)
    MAGAZINE=$(echo "$OUTPUT" | grep -i "magazine" | cut -d ':' -f2- | tr -d '[]' | tr '\n' ' ')

    # Salva resultado
    echo "$run,$INST,$CUSTO,$TEMPO,$SLOTS,$FERRAMENTAS,\"$MAGAZINE\",\"$PARAMS\"" >> "$OUTFILE"
  done
done

