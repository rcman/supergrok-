let player, inventory = {
    axe: 1,
    pickaxe: 1,
    knife: 1,
    canteen: 1,
    wood: 0,
    stone: 0,
    fiber: 0,
    scrap: 0
};

function initPlayer() {
    const geometry = new THREE.BoxGeometry(1, 2, 1);
    const material = new THREE.MeshStandardMaterial({ color: 0xff0000 });
    player = new THREE.Mesh(geometry, material);
    player.position.set(0, 10, 0);
    scene.add(player);

    // Keyboard controls
    document.addEventListener('keydown', (event) => {
        if (event.key === 'i') toggleInventory();
        if (event.key === 'b') toggleBuilding();
    });
}

function updatePlayer() {
    // Player movement (placeholder)
}

function toggleInventory() {
    const invDiv = document.getElementById('inventory');
    invDiv.style.display = invDiv.style.display === 'none' ? 'block' : 'none';
    invDiv.innerHTML = JSON.stringify(inventory, null, 2);
}