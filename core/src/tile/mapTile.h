#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "tileID.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>

class Label;
class Labels;
class MapProjection;
class Style;
class TextBuffer;
class VboMesh;
class View;


enum class TileState { none, loading, processing, ready, canceled };

/* Tile of vector map data
 * 
 * MapTile represents a fixed area of a map at a fixed zoom level; It contains its position within a quadtree of
 * tiles and its location in projected global space; It stores drawable geometry of the map features in its area
 */
class MapTile {

public:
  
    MapTile(TileID _id, const MapProjection& _projection);


    virtual ~MapTile();

    /* Returns the immutable <TileID> of this tile */
    const TileID& getID() const { return m_id; }

    /* Returns the center of the tile area in projection units */
    const glm::dvec2& getOrigin() const { return m_tileOrigin; }
    
    /* Returns the map projection with which this tile interprets coordinates */
    const MapProjection* getProjection() const { return m_projection; }
    
    /* Returns the length of a side of this tile in projection units */
    float getScale() const { return m_scale; }
    
    /* Returns the reciprocal of <getScale()> */
    float getInverseScale() const { return m_inverseScale; }
    
    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }

    /* Adds drawable geometry to the tile and associates it with a <Style>
     * 
     * Use std::move to pass in the mesh by move semantics; Geometry in the mesh
     * must have coordinates relative to the tile origin.
     */
    void addGeometry(const Style& _style, std::shared_ptr<VboMesh> _mesh);
    
    void addLabel(const std::string& _styleName, std::shared_ptr<Label> _label);
    
    std::shared_ptr<VboMesh>& getGeometry(const Style& _style);

    /* uUdate the Tile considering the current view */
    void update(float _dt, const View& _view);

    /* Update labels position considering the tile transform */
    void updateLabels(float _dt, const Style& _style, const View& _view);
    
    /* Push the label transforms to the font rendering context */
    void pushLabelTransforms(const Style& _style, std::shared_ptr<Labels> _labels);

    void setTextBuffer(const Style& _style, std::shared_ptr<TextBuffer> _buffer);
    std::shared_ptr<TextBuffer> getTextBuffer(const Style& _style) const;

    /* Draws the geometry associated with the provided <Style> and view-projection matrix */
    void draw(const Style& _style, const View& _view);
    
    /* 
     * Methods to set and get proxy counter
     */
    int getProxyCounter() { return m_proxyCounter; }
    void incProxyCounter() { m_proxyCounter++; }
    void decProxyCounter() { m_proxyCounter = m_proxyCounter > 0 ? m_proxyCounter - 1 : 0; }
    void resetProxyCounter() { m_proxyCounter = 0; }

    enum ProxyID {
        no_proxies = 0,
        child1 = 1 << 0,
        child2 = 1 << 1,
        child3 = 1 << 2,
        child4 = 1 << 3,
        parent = 1 << 4,
        parent2 = 1 << 5,
    };

    bool setProxy(ProxyID id) {
        if ((m_proxies & id) == 0) {
            m_proxies |= id;
            return true;
        }
        return false;
    }

    bool unsetProxy(ProxyID id) {
        if ((m_proxies & id) != 0) {
            m_proxies &= ~id;
            return true;
        }
        return false;
    }

    bool isCanceled() const {
        return m_state == TileState::canceled;
    }

    bool isReady() const {
        return m_state == TileState::ready;
    }

    /* Method to check whther this tile is in the current set of visible tiles
     * determined by view::updateTiles().
     */
    bool isVisible() const {
        return m_visible;
    }

    void setVisible(bool _visible) {
         m_visible = _visible;
    }

    double getPriority() const {
        return m_priority.load();
    }

    void setPriority(double _priority) {
        m_priority.store(_priority);
    }

    bool hasState(TileState _state) {
        return (m_state == _state);
    }

    TileState getState() {
        return m_state;
    }

    void setState(TileState _state) {
        m_state = _state;
    }

private:

    const TileID m_id;

    const MapProjection* m_projection = nullptr;

    /* A Counter for number of tiles this tile acts a proxy for */
    int m_proxyCounter = 0;

    uint8_t m_proxies = 0;

    /* The loading state of the tile.
     * NB: This may be moved to TileTask when multiple DataSources should
     *     contribute to a single tile.
     */
    TileState m_state = TileState::none;

    bool m_visible;

    float m_scale = 1;

    float m_inverseScale = 1;

    std::atomic<double> m_priority;

    glm::dvec2 m_tileOrigin; // Center of the tile in 2D projection space in meters (e.g. mercator meters)

    glm::mat4 m_modelMatrix; // Matrix relating tile-local coordinates to global projection space coordinates;
    // Note that this matrix does not contain the relative translation from the global origin to the tile origin.
    // Distances from the global origin are too large to represent precisely in 32-bit floats, so we only apply the
    // relative translation from the view origin to the model origin immediately before drawing the tile.

    std::unordered_map<std::string, std::shared_ptr<VboMesh>> m_geometry; // Map of <Style>s and their associated <VboMesh>es
    std::unordered_map<std::string, std::vector<std::shared_ptr<Label>>> m_labels;
    std::map<std::string, std::shared_ptr<TextBuffer>> m_buffers; // Map of <Style>s and the associated text buffer

};
