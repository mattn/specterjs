// Find pizza in New York using Google Local

if (specter.state.length === 0) {
    specter.state = 'ピザ';
    specter.open('http://www.google.com/m/local?site=local&q=pizza+in+osaka');
} else {
    [].forEach.call(document.querySelectorAll('div.bf'),
        function(n) { console.log(n.innerText); });
    specter.exit();
}
