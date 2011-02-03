// Find pizza in New York using Google Local

if (specter.state.length === 0) {
    specter.state = 'ピザ';
    specter.open('http://www.google.com/m/local?site=local&q=pizza+in+osaka');
} else {
    var list = document.querySelectorAll('div.bf');
    for (var i in list) {
        console.log(list[i].innerText);
    }
    specter.exit();
}
