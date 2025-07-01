#!/bin/bash

# Caminho para o executável
BIN="./binTIP"
OUTFILE="resultados.csv"

# Parâmetros da elite
PARAMS="--TEMP_INIT 0.02 --TEMP_FIM 10.0 --MCL 300 --TEMP_DIST 2 --TYPE_UPDATE 3 --TEMP_UPDATE 5 --MOV_TYPE 3"

# Cabeçalho, só adiciona se o arquivo não existe
if [ ! -f "$OUTFILE" ]; then
  echo "Execução,Instância,Custo,Tempo,Slots,Ferramentas,Magazine,Ciclo,Parametros" > "$OUTFILE"
fi

# Para cada instância na pasta
for INSTANCE in instances/*.txt; do
  INST=$(basename "$INSTANCE")
  echo "Rodando $INST..."

  for run in {1..10}; do
    OUTPUT=$($BIN "$INSTANCE" $PARAMS)

    # Extrações com grep/sed
    CUSTO=$(echo "$OUTPUT" | grep -i "Custo" | grep -Eo '[0-9]+')
    TEMPO=$(echo "$OUTPUT" | grep -i "Tempo" | grep -Eo '[0-9.]+' | head -n 1)
    SLOTS=$(echo "$OUTPUT" | grep -i "slots" | grep -Eo '[0-9]+' | head -n 1)
    FERRAMENTAS=$(echo "$OUTPUT" | grep -i "ferramenta" | grep -Eo '[0-9]+' | head -n 1)
    MAGAZINE=$(echo "$OUTPUT" | grep -i "magazine" | cut -d ':' -f2- | tr -d '[]' | tr '\n' ' ')
    CICLO=$(echo "$OUTPUT" | grep -i "Ciclo" | grep -Eo '[0-9]+')

    # Salva resultado
    echo "$run,$INST,$CUSTO,$TEMPO,$SLOTS,$FERRAMENTAS,$CICLO,\"$MAGAZINE\",\"$PARAMS\"" >> "$OUTFILE"
  done
done

