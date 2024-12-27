function eventHandler(e) {
    console.log(`Event: ${e}`);
}


function loadMenu(){
    fetch("./menu.html")
    .then((response) => {
        if (response.ok)
            return response.text();
        else {
            throw new Error(response.statusText);
        }
    })
    .then((body) => {
        document.getElementById('menubar').innerHTML = body;
    })
    .catch(error => {
        console.log("Cannot load menu \n" + error);
    });
}