function initTerrain() {
    // Create terrain geometry
    const geometry = new THREE.PlaneGeometry(1000, 1000, 50, 50);
    const material = new THREE.MeshStandardMaterial({ color: 0x228B22 });
    const terrain = new THREE.Mesh(geometry, material);
    terrain.rotation.x = -Math.PI / 2;
    scene.add(terrain);

    // Add elevation using Perlin noise (placeholder)
    const vertices = geometry.attributes.position.array;
    for (let i = 0; i < vertices.length; i += 3) {
        vertices[i + 2] = Math.random() * 10; // Basic elevation
    }
    geometry.attributes.position.needsUpdate = true;

    // Add water
    const waterGeometry = new THREE.PlaneGeometry(1000, 1000);
    const water = new THREE.Water(waterGeometry, {
        textureWidth: 512,
        textureHeight: 512,
        waterNormals: new THREE.TextureLoader().load('https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/waternormals.jpg'),
        alpha: 1.0,
        sunDirection: new THREE.Vector3(),
        sunColor: 0xffffff,
        waterColor: 0x001e0f,
        distortionScale: 3.7,
    });
    water.rotation.x = -Math.PI / 2;
    water.position.y = -2;
    scene.add(water);
}