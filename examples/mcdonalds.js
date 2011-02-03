if (specter.state.length === 0) {
    specter.state = 'mcdonalds';
    specter.open('http://www.mcdonalds.co.jp/menu/regular/index.html');
} else {
    [].forEach.call(document.querySelectorAll('ul.food-set>li img'),
        function(n) { console.log(n.getAttribute('alt')); });
    specter.exit();
}
