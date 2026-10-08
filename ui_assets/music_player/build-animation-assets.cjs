const fs = require('fs');
const path = require('path');
const sharp = require(process.env.PET_SHARP_PATH || 'sharp');
const root = path.resolve(__dirname, '../../ui_lvgl/dashboard_1024x600_v1/import/image/music');

/** Build small runtime assets while keeping the approved originals. */
async function build() {
    const dog = process.argv[2];
    if (dog) await sharp(dog).resize(216, 142, {fit: 'fill'}).png().toFile(path.join(root, 'dog_headphones_clean.png'));
    await sharp(path.join(root, 'die.png')).resize(96, 96).png().toFile(path.join(root, 'die_display.png'));
    const shape = '<path d="M11 5 Q11 2 14 3 L21 7 Q24 10 23 15 L19 15 Q20 11 15 10 L15 23 Q15 29 8 30 Q1 31 1 26 Q1 21 11 22 Z" fill="#21d7e7"/>';
    for (const [name, mirrored] of [['music_note_right', false], ['music_note_left', true]]) {
        const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="32" viewBox="0 0 24 32"><g${mirrored ? ' transform="translate(24 0) scale(-1 1)"' : ''}>${shape}</g></svg>`;
        fs.writeFileSync(path.join(__dirname, 'source', name + '.svg'), svg);
        await sharp(Buffer.from(svg)).png().toFile(path.join(root, name + '.png'));
    }
}
build().catch(error => {console.error(error); process.exitCode = 1;});
