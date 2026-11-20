function enviar() {
    //JavaScript

    //Pega o ip digitado
    let ip = document.getElementById("ip").value;

    // Pega a mensagem digitada
    let msg = document.getElementById("mensagem").value;

    //Coloca no campo de resposta que a mensagem vai ser enviada para a placa


    document.getElementById("resposta").value = "aguarda resposta do NIOS"

    fetch("http://" + ip + "/", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: msg
    })
        .then(respostaDaRede => respostaDaRede.text())
        .then(dadosRecebidos => {
            document.getElementById("resposta").value = dadosRecebidos;
        })
        .catch(erro => {
            document.getElementById("resposta").value = "Erro ao conectar na placa";
        });

}