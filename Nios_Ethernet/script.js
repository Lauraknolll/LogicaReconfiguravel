function enviar() {
    //JavaScipt

    //Pega o ip digitado
    let ip = document.getElementById("ip").value;

    // Pega a mensagem digitada
    let msg = document.getElementById("mensagem").value;

    //Monta a requisição http
    let url = "http://" + ip + "/?msg=" + encodeURIComponent(msg)//substitui espaços em branco e caracteries especiais por codigos de web para não quebrar o texto;

    //Coloca no campo de resposta que a mensagem vai ser enviada para a placa


    document.getElementById("resposta").value = "aguarda resposta do NIOS"

    fetch(url)
        .then(respostaDaRede => respostaDaRede.text())//then bota o programa pra dormir esperando a conexão
        .then(dadosRecebidos => {
            document.getElementById("resposta").value = dadosRecebidos;
        })
        .catch(erro => { // se der erro ele  avisa 
            document.getElementById("resposta").value = "Erro ao conectar na placa";
            //console.error(erro)
        })

}