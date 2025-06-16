let scene, camera, renderer, controls;

function init() {
    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    renderer = new THREE.WebGLRenderer();
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    // Add fog
    scene.fog = new THREE.Fog(0xaaaaaa, 50, 200);

    // Initialize terrain
    initTerrain();

    // Initialize player
    initPlayer();

    // Initialize world objects
    initWorldObjects();

    // Camera controls
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    camera.position.set(0, 50, 100);
    controls.update();

    animate();
}

function animate() {
    requestAnimationFrame(animate);
    updatePlayer();
    updateWorldObjects();
    renderer.render(scene, camera);
}

window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
});

init();