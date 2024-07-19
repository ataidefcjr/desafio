
## Introdução

**`Bot`** criado para monitorar a carteira solicitada em busca da sua chave pública, iniciar a quebra usando Kangaroo ou Collider no **`Windows`**

**`ATENÇÃO:`** Recomendo a utilização do Kangaroo em primeiro caso, alem de ser mais rápido em meus testes iniciais, o código é mais confiável e está disponível em https://github.com/JeanLucPons/Kangaroo.

O meu código está livre para ser alterado como desejar.

## Instalação
Você precisa ter instalado o Python e o Git.
Para instalá-los via terminal pode usar o seguinte comando: `winget install Git.Git` e `winget install python` durante a instalação do python é necessário marcar a caixa de confirmação `Adicionar ao PATH` e ao finalizar a instalação feche o terminal e abra novamente para continuar os proximos comandos. 

Se você ja tem o git e python instalados `Abra o terminal e execute os seguintes comandos:`


```
git clone https://github.com/ataidefcjr/desafio
cd desafio
pip install -r requirements.txt
```
## Como usar
Abra o terminal, navegue até a pasta `cd desafio` e execute o comando `python main.py`

Será solicitado o endereço da carteira a ser monitada e o endereço da sua carteira para tentiva de transferencia, quando encontrado a chave pública será iniciado a quebra de acordo com a sua escolha.(Kangaroo ou Collider)

Se deseja usar apenas **`CPU`**, selecione Kangaroo, então será solicitado se deseja usar `GPU` ou não.

Por padrão o Kangaroo ira utilizar todos os threads disponíveis, se deseja alterar, abra o hashs.py e altere o script na linha 50 de acordo com sua preferencia.

## Observações Importantes:
* O range da chave privada foi limitado para as carteiras `65 e 66` do puzzle, ajuste em hashs.py linhas 10 e 11 se necessário.

* Se for utilizar o Collider execute uma vez e interrompa, ele vai sugerir os melhores valores, basta alterar a variável argumentos em hashs.py na linha 12

* Ambos são executáveis já compilados, recomendo o bloqueio no firewall dos respectivos executáveis.

* Ao efetivar a quebra o bot irá tentar enviar os fundos para sua carteira usando a biblioteca bit, porém em teste realizado durante a live do desafio no canal https://www.youtube.com/@investidorint não funcionou como o esperado, pelo menos nesse caso em específico, mostrando saldo 0 para o endereço.

* Tentei tambem importar na carteira Electrum logo que a transferencia do bot falhou (cerca de 5 segundos) e tambem acusou saldo 0 e não permitiu iniciar transferência.


* A `WIF` será exibida no console assim que quebrada a criptografia para que possa tentar por outros meios.

* Alguns arquivos ficarão salvos no diretório do repositório após encontrar alguma chave.`PublicKey.txt` contendo as chaves públicas encontradas e `WIF.txt` com as chaves WIF encontradas.

## Doações

* Se conseguir resgatar e quiser doar alguns BTC pro papai aqui: `bc1qych3lyjyg3cse6tjw7m997ne83fyye4des99a9`