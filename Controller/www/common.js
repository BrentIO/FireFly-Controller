function eventHandler(e) {
    console.log(`Event: ${e}`);
    showToast(e);
}


function errorHandler(e) {
    console.error(`Error: ${e}`);
    showToast(e, "danger");
}


function warningHandler(e) {
    console.warn(`Warning: ${e}`);
    showToast(e, "warn");
}


function showToast(message, type='notification', options = {}) {

    if(document.getElementById("toastContainer") === null){
        const toastContainer = document.createElement('div');
        toastContainer.classList.add("toast-container");
        toastContainer.classList.add("top-0");
        toastContainer.classList.add("end-0");
        toastContainer.classList.add("p-3");
        toastContainer.setAttribute("id", "toastContainer");
        document.body.appendChild(toastContainer);
    }

    const toastContainer = document.getElementById("toastContainer");
    const toastInstance = document.createElement('div');
    toastInstance.classList.add('toast');
    toastInstance.classList.add('align-items-center');
    toastInstance.classList.add('border-0');
    toastInstance.setAttribute("role", "alert");
    toastInstance.setAttribute("aria-live", "assertive");
    toastInstance.setAttribute("aria-atomic", "true");

    switch(type){
        case 'danger':
        case 'error':
            toastInstance.classList.add("text-bg-danger");
            options['autohide'] = false;
            break;
        
        case 'warn':
        case 'warning':
            toastInstance.classList.add("text-bg-warning");
            options['delay'] = 5000;
            break;

        default:
            toastInstance.classList.add("text-bg-primary");
            options['delay'] = 2500;
            break;
    }

    toastInner = document.createElement('div');
    toastInner.classList.add('d-flex');

    const toastBody = document.createElement('div');
    toastBody.classList.add('toast-body');
    toastBody.innerText = message;
    toastInner.appendChild(toastBody);

    const toastButton = document.createElement('button');
    toastButton.setAttribute("type", "button");
    toastButton.classList.add('btn-close');
    toastButton.classList.add('btn-close-white');
    toastButton.classList.add('me-2');
    toastButton.classList.add('m-auto');
    toastButton.setAttribute("data-bs-dismiss", "toast");
    toastButton.setAttribute("aria-label", "Close");
    toastInner.appendChild(toastButton);

    toastInstance.appendChild(toastInner);
    toastContainer.appendChild(toastInstance);

    const thisToast = new bootstrap.Toast(toastInstance, options);
    thisToast.show();
}


function loadMenu(menu=null, menuItem=null){
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
        if(menu != null){
            new bootstrap.Collapse(document.getElementById(menu), {toggle: true});
        }

        if(menuItem != null){
            document.getElementById(menuItem).classList.add("activeMenuItem");
        }
    })
    .catch(error => {
        errorHandler("Cannot load menu \n" + error);
    });
}


async function exportConfig(){
    try {
        const blob = await db.export();
        download(blob, `FireFlyConfig_${Date.now()}.json`, "application/json");
        eventHandler("Export successful!");
    } catch (error) {
        errorHandler(error);
    }
}


async function importConfig(file){
    try {
        await db.delete();
        db = await Dexie.import(file);
        eventHandler("Import successful!");
    } catch (error) {
        errorHandler(error);
    }
}


class controllerLocalStorage{
    ip;
    certificates = [];
    firmwareVersion;
    uiVersion;
    visualToken;
    uuid;

    constructor(id){
        this.id = id;
        this.retrieve();
    }

    get ip(){
        return this.ip;
    }

    set ip(value){
        this.ip = value;
    }

    get certificates(){
        return this.certificates;
    }

    set certificates(value){
        this.certificates = value;
    }

    get firmwareVersion(){
        return this.firmwareVersion;
    }

    set firmwareVersion(value){
        this.firmwareVersion = value;
    }

    get uuid(){
        return this.uuid;
    }

    set uuid(value){
        this.uuid = value;
    }

    get uiVersion(){
        return this.uiVersion;
    }

    set uiVersion(value){
        this.uiVersion = value;
    }

    get visualToken(){
        return this.visualToken;
    }

    set visualToken(value){
        this.visualToken = value;
    }

    retrieve(){
        const record = JSON.parse(localStorage.getItem("controller_" + this.id));

        if(record == null){
            return;
        }

        if("ip" in record){
            this.ip = record.ip;
        }

        if("certificates" in record){
            this.certificates = record.certificates;
        }

        if("firmwareVersion" in record){
            this.firmwareVersion = record.firmwareVersion;
        }

        if("uiVersion" in record){
            this.uiVersion = record.uiVersion;
        }
    }

    save(){
        localStorage.setItem("controller_" + this.id, JSON.stringify(this));
    }

    delete(){
        localStorage.removeItem("controller_" + this.id);
    }

}


async function storeCertificate(filename, certificate){

    if(await db.certificates.where({certificate: certificate}).count() > 0){
        return;
    }

    truncatedCert = certificate.replace("-----BEGIN CERTIFICATE-----", "");
    truncatedCert = truncatedCert.replace("-----END CERTIFICATE-----", "");

    try {
        decoded = decodeCert(truncatedCert);

        await db.certificates.put({
            fileName: filename,
            certificate: certificate,
            commonName: decoded['commonName-X520'],
            expiration: decoded['UTCTime'],
            countryName: decoded['countryName-X520'],
            encryptionType: decoded['encryptionAlgo'],
            locality: decoded['localityName-X520'],
            organization: decoded['organizationName-X520'],
            organizationalUnitName: decoded['organizationalUnitName-X520'],
            stateOrProvinceName: decoded['stateOrProvinceName-X520']
        });
    }catch (error){
        errorHandler(error);
    }
}


async function exportCertificate(id){
    try {
        result = await db.certificates.where({id:id}).toArray();
        if(result.length != 1){
            throw new Error("Unexpected number of results when exporting.");
        }
        download(result[0].certificate, `${result[0].fileName}`, "application/x-x509-ca-cert");
    } catch (error) {
        errorHandler(error);
    }
}


async function checkIfInUse_breaker(id){

    const breakers = await db.breakers.where('id').equals(id).toArray();

    if(breakers.length == 0){
        return false;
    }

    await Promise.all(breakers.map(async breaker => {
        [breaker.id] = await Promise.all([
            db.circuits.where('breaker').equals(breaker.id).first()
        ])
    }));

    if(breakers[0].id == undefined){
        return false;
    }
    else{
        return true;
    }
}