const recipes = {
    rope: { fiber: 5 },
    campfire: { wood: 10, stone: 5 },
    craftingTable: { wood: 20, rope: 2 },
    forge: { stone: 30, scrap: 10 }
};

function craftItem(item) {
    const recipe = recipes[item];
    for (let resource in recipe) {
        if (inventory[resource] < recipe[resource]) return false;
    }
    for (let resource in recipe) {
        inventory[resource] -= recipe[resource];
    }
    inventory[item] = (inventory[item] || 0) + 1;
    return true;
}