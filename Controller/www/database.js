let dbVersion = 2;

const dbName = "FireFly-Controller";
let db = new Dexie(dbName);


const initDB = () => {
    db.open();
};


async function exportConfig(){
    try {
        const blob = await db.export({prettyJson: true});
        download(blob, `FireFlyConfig_${Date.now()}.json`, "application/json");
    } catch (error) {
        eventHandler(error);
    }
}


async function importConfig(file){
    try {
        await db.delete();
        db = await Dexie.import(file);
    } catch (error) {
        eventHandler(error);
    }
}