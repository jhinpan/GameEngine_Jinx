const express = require('express');
const path = require('path');
const fs = require('fs');
const bodyParser = require('body-parser');

const app = express();
const PORT = 3000;

app.use(bodyParser.json());
app.use(express.static(path.join(__dirname)));

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'game_engine_web.html'));
});

app.post('/update-map', (req, res) => {
    const { mapData } = req.body;
    const fullPath = path.join(__dirname, 'resources/component_types/GameManager.lua');

    fs.readFile(fullPath, 'utf8', (err, data) => {
        if (err) {
            console.error('Failed to read GameManager.lua:', err);
            return res.status(500).send({message: 'Failed to read existing GameManager script'});
        }

        const startTag = '-- BEGIN stage1';
        const endTag = '-- END stage1';
        const start = data.indexOf(startTag) + startTag.length;
        const end = data.indexOf(endTag);

        if (start === -1 || end === -1) {
            console.error('Tags not found in GameManager.lua.');
            return res.status(500).send({message: 'Tags not found in GameManager.lua'});
        }

        // Ensure we include the entire stage1 definition in the replacement
        const newData = data.substring(0, start) + `\n    stage1 = \n        ${mapData}\n    ,\n` + data.substring(end);

        fs.writeFile(fullPath, newData, 'utf8', (err) => {
            if (err) {
                console.error('Failed to update GameManager.lua:', err);
                return res.status(500).send({message: 'Failed to update GameManager script'});
            }

            console.log('Map data written successfully.');
            res.send({message: 'Map updated successfully'});
        });
    });
});

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
