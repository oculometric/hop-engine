#include "mesh.h"
#include "package.h"

#include <charconv>

using namespace HopEngine;

struct FaceCorner
{
    uint16_t co;
    uint16_t uv;
    uint16_t vn;
};

struct FaceCornerReference
{
    uint16_t normal_index;
    uint16_t uv_index;
    uint16_t transferred_vert_index;
};

static glm::vec3 computeTangent(glm::vec3 co_a, glm::vec3 co_b, glm::vec3 co_c, glm::vec2 uv_a,
    glm::vec2 uv_b, glm::vec2 uv_c)
{
    // vector from the target vertex to the second vertex
    glm::vec3 ab = { co_b.x - co_a.x, co_b.y - co_a.y, co_b.z - co_a.z };
    // vector from the target vertex to the third vertex
    glm::vec3 ac = { co_c.x - co_a.x, co_c.y - co_a.y, co_c.z - co_a.z };
    // delta uv between target and second
    glm::vec2 uv_ab = { uv_b.x - uv_a.x, uv_b.y - uv_a.y };
    // delta uv between target and third
    glm::vec2 uv_ac = { uv_c.x - uv_a.x, uv_c.y - uv_a.y };
    // matrix representing UVs
    glm::mat3 uv_mat = glm::mat3(glm::vec3(uv_ab, 0), glm::vec3(uv_ac, 0), { 0, 0, 1 });
    // matrix representing vectors between vertices
    glm::mat3 vec_mat = glm::mat3(ab, ac, { 0, 0, 0 });

    // we should be able to express the vectors from A->B and A->C with reference to the difference in UV
    // coordinate and the tangent and bitangent:
    //
    // AB = (duv_ab.x * T) + (duv_ab.y * B)
    // AC = (duv_ac.x * T) + (duv_ac.y * B)
    //
    // this gives us 6 simultaneous equations for the XYZ coordinates of the tangent and bitangent
    // these can be expressed and solved with matrices:
    //
    // [ AB.x  AC.x  0 ]     [ T.x  B.x  N.x ]   [ duv_ab.x  duv_ac.x  0 ]
    // [ AB.y  AC.y  0 ]  =  [ T.y  B.y  N.y ] * [ duv_ab.y  duv_ac.y  0 ]
    // [ AB.z  AC.z  0 ]     [ T.z  B.z  N.z ]   [ 0         0         1 ]
    //

    vec_mat = vec_mat * glm::inverse((uv_mat));

    return glm::normalize(glm::vec3{ vec_mat[0] }); // extract tangent
}

bool isWhitespace(char c) { return (c == ' ' || c == '\r' || c == '\n' || c == '\t'); }
bool isNewline(char c) { return (c == '\r' || c == '\n'); }

bool Mesh::readOBJ(const DataBlock& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds)
{
    // vectors to load data into
    std::vector<std::pair<glm::vec3, glm::vec3>> tmp_position_colour;
    std::vector<glm::vec3> tmp_normal;
    std::vector<glm::vec2> tmp_texcoord;
    std::vector<FaceCorner> tmp_corner;

    // parsing state
    std::string tmp_str;
    tmp_str.reserve(32);
    size_t i;

    auto getChunk = [&]() -> void
    {
        tmp_str.clear();
        bool started = false;
        while (i < data.size() && !isNewline(data[i]))
        {
            if (isWhitespace(data[i]))
            {
                if (started) return;
                else
                {
                    ++i;
                    continue;
                }
            }
            else
            {
                started = true;
                tmp_str.push_back(data[i]);
                ++i;
            }
        }
    };

    auto splitCorner = [&]() -> FaceCorner
    {
        FaceCorner fci = { 0, 0, 0 };

        const size_t first_break_ind = tmp_str.find('/');
        std::from_chars(tmp_str.data(), tmp_str.data() + glm::min(first_break_ind, tmp_str.size()), fci.co);
        --fci.co;
        if (first_break_ind == std::string::npos) return fci;

        const size_t second_break_ind = tmp_str.find('/', first_break_ind + 1);
        if (second_break_ind != first_break_ind + 1)
        {
            std::from_chars(tmp_str.data() + first_break_ind + 1,
                tmp_str.data() + glm::min(second_break_ind, tmp_str.size()), fci.uv);
            --fci.uv;
        }
        if (second_break_ind == std::string::npos) return fci;
        std::from_chars(tmp_str.data() + second_break_ind + 1, tmp_str.data() + tmp_str.size(), fci.vn);
        --fci.vn;

        return fci;
    };

    for (i = 0; i < data.size(); ++i)
    {
        char c = static_cast<char>(data[i]);
        // if we detect a comment, skip to the next line
        if (c == '#')
        {
            while (i < data.size() && !isNewline(data[i])) ++i;
            continue;
        }

        // if we're looking for the start of a command, skip until we see something of interest
        if (isWhitespace(c)) continue;
        if (c == 'v' && (i + 1) < data.size() && isWhitespace(data[i + 1]))
        {
            // we found a vertex command
            ++i;
            glm::vec3 position = { 0, 0, 0 };
            glm::vec3 colour   = { 1, 1, 1 };

            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), position.x);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), position.y);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), position.z);

            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), colour.x);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), colour.y);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), colour.z);

            tmp_position_colour.emplace_back(position, colour);
            continue;
        }
        else if (c == 'f' && (i + 1) < data.size() && isWhitespace(data[i + 1]))
        {
            // we found a face corner command
            ++i;
            getChunk();
            tmp_corner.push_back(splitCorner());
            getChunk();
            tmp_corner.push_back(splitCorner());
            getChunk();
            tmp_corner.push_back(splitCorner());
            continue;
        }
        else if (c == 'v' && (i + 2) < data.size() && data[i + 1] == 'n' && isWhitespace(data[i + 2]))
        {
            // we found a vertex normal command
            i += 2;
            glm::vec3 normal = { 0, 0, 0 };
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), normal.x);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), normal.y);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), normal.z);

            tmp_normal.emplace_back(normal);
            continue;
        }
        else if (c == 'v' && (i + 2) < data.size() && data[i + 1] == 't' && isWhitespace(data[i + 2]))
        {
            // we found a texture coordinate command
            i += 2;
            glm::vec2 texcoord = { 0, 0 };
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), texcoord.x);
            getChunk();
            std::from_chars(tmp_str.data(), tmp_str.data() + tmp_str.size(), texcoord.y);

            tmp_texcoord.emplace_back(texcoord);
            continue;
        }
        else if ((c == 'o' || c == 's') && (i + 1) < data.size() && isWhitespace(data[i + 1]))
        {
            // we found an object command. ignore it
            while (i < data.size() && data[i] != '\r' && data[i] != '\n') ++i;
            continue;
        }
        else
        {
            DBG_ERROR("error reading OBJ file, invalid command");
            return false;
        }
    }

    // for each coordinate, stores a list of all the times it has been used by a face corner, and what the
    // normal/uv index was for that face corner this allows us to tell when we should split a vertex (i.e.
    // if it has already been used by another face corner but which had a different normal and/or a
    // different uv)
    std::vector<std::vector<FaceCornerReference>> fc_normal_uses(tmp_position_colour.size(),
        std::vector<FaceCornerReference>());

    verts.clear();
    inds.clear();

    for (auto [co, uv, vn] : tmp_corner)
    {
        bool found_matching_vertex = false;
        uint16_t match             = 0;
        for (auto [normal_index, uv_index, transferred_vert_index] : fc_normal_uses[co])
        {
            if (normal_index == vn && uv_index == uv)
            {
                found_matching_vertex = true;
                match                 = transferred_vert_index;
                break;
            }
        }

        if (found_matching_vertex) inds.push_back(match);
        else
        {
            Vertex new_vert;
            new_vert.position = glm::vec4(tmp_position_colour[co].first, 1);
            new_vert.colour   = glm::vec4(tmp_position_colour[co].second, 0);
            if (vn < tmp_normal.size()) new_vert.normal = glm::vec4(tmp_normal[vn], 0);
            if (uv < tmp_texcoord.size()) new_vert.uv = glm::vec3{ tmp_texcoord[uv], 0 };

            uint16_t new_index = static_cast<uint16_t>(verts.size());
            fc_normal_uses[co].push_back(FaceCornerReference{ vn, uv, new_index });

            inds.push_back(new_index);
            verts.push_back(new_vert);
        }
    }

    if (tmp_normal.empty())
    {
        for (size_t i = 0; i < inds.size() - 2; i += 3)
        {
            const uint16_t i0 = inds[i];
            const uint16_t i1 = inds[i + 1];
            const uint16_t i2 = inds[i + 2];

            const glm::vec3 v0 = verts[i0].position;
            const glm::vec3 v1 = verts[i1].position;
            const glm::vec3 v2 = verts[i2].position;

            glm::vec3 e01    = v1 - v0;
            glm::vec3 e02    = v2 - v0;
            glm::vec4 normal = glm::vec4(glm::cross(e01, e02), 0);

            verts[i0].normal += normal;
            verts[i1].normal += normal;
            verts[i2].normal += normal;
        }

        for (Vertex& vert : verts) vert.normal = glm::normalize(vert.normal);
    }

    // compute tangents
    std::vector<bool> touched = std::vector<bool>(verts.size(), false);
    for (uint32_t tri = 0; tri < inds.size() / 3; tri++)
    {
        uint16_t v0 = inds[(tri * 3) + 0];
        Vertex f0   = verts[v0];
        uint16_t v1 = inds[(tri * 3) + 1];
        Vertex f1   = verts[v1];
        uint16_t v2 = inds[(tri * 3) + 2];
        Vertex f2   = verts[v2];

        if (!touched[v1])
            verts[v1].tangent =
                glm::vec4(computeTangent(f1.position, f0.position, f2.position, f1.uv, f0.uv, f2.uv), 1);
        if (!touched[v2])
            verts[v2].tangent =
                glm::vec4(computeTangent(f2.position, f0.position, f1.position, f2.uv, f0.uv, f1.uv), 1);
        if (!touched[v0])
            verts[v0].tangent =
                glm::vec4(computeTangent(f0.position, f1.position, f2.position, f0.uv, f1.uv, f2.uv), 1);

        touched[v0] = true;
        touched[v1] = true;
        touched[v2] = true;
    }

    // transform from Z back Y up space into Z up Y forward space
    for (Vertex& fv : verts)
    {
        fv.position = { fv.position.x, -fv.position.z, fv.position.y, 1 };
        fv.normal   = { fv.normal.x, -fv.normal.z, fv.normal.y, 0 };
        fv.tangent  = { fv.tangent.x, -fv.tangent.z, fv.tangent.y, 0 };
    }

    return true;
}