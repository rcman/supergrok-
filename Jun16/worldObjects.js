function initWorldObjects() {
    // Add trees
    for (let i = 0; i < 1000; i++) {
        const geometry = new THREE.CylinderGeometry(1, 1, 10, 8);
        const material = new THREE.MeshStandardMaterial({ color: 0x8B4513 });
        const tree = new THREE.Mesh(geometry, material);
        tree.position.set(
            Math.random() * 1000 - 500,
            5,
            Math.random() * 1000 - 500
        );
        scene.add(tree);
    }

    // Add rocks, animals, barrels, buildings (placeholders)
}