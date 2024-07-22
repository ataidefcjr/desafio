
## Introdução

**`Bot`** criado para monitorar a carteira solicitada em busca da sua chave pública, iniciar a quebra usando Kangaroo no **`Windows`**

**`ATENÇÃO:`** Baixe o executavel do Kangaroo em https://github.com/JeanLucPons/Kangaroo/releases/tag/2.2 ou compile por conta própria.

O meu código está livre para ser alterado como desejar.

## Instalação
Você precisa ter instalado o Python e o Git.
Para instalá-los via terminal pode usar o seguinte comando: `winget install Git.Git` e `winget install python` durante a instalação do python é necessário marcar a caixa de confirmação `Adicionar ao PATH` e ao finalizar a instalação feche o terminal e abra novamente para continuar os proximos comandos. 

Se você ja tem o git e python instalados `Abra o terminal e execute os seguintes comandos:`


```
git clone https://github.com/ataidefcjr/desafio
cd desafio
pip install -r requirements.txt
python main.py
```
## Como usar
Abra o terminal, navegue até a pasta `cd desafio` e execute o comando `python main.py`

Será solicitado o endereço da carteira a ser monitada e o endereço da sua carteira para tentiva de transferencia, quando encontrado a chave pública será iniciado a quebra da chave publica usando Kangaroo.

Por padrão o Kangaroo ira utilizar todos os threads disponíveis, se deseja alterar, abra o hashs.py e altere o script na linha 34 de acordo com sua preferencia.

## Observações Importantes:

* Ao efetivar a quebra o bot irá tentar enviar os fundos para sua carteira usando a biblioteca bit, porém em teste realizado durante a live do desafio no canal https://www.youtube.com/@investidorint não funcionou como o esperado, pelo menos nesse caso em específico, mostrando saldo 0 para o endereço.

* Tentei tambem importar na carteira Electrum logo que a transferencia do bot falhou (cerca de 5 segundos) e tambem acusou saldo 0 e não permitiu iniciar transferência.

* A `WIF` será exibida no console assim que quebrada a criptografia para que possa tentar por outros meios.

* Alguns arquivos ficarão salvos no diretório do repositório após encontrar alguma chave.`PublicKey.txt` contendo as chaves públicas encontradas e `WIF.txt` com as chaves WIF encontradas.

## Doações

* Se conseguir resgatar e quiser doar alguns BTC pro papai aqui: `bc1qych3lyjyg3cse6tjw7m997ne83fyye4des99a9`