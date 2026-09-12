/***********************************************************************************
**
** GLVertexBatch.cpp
**
** See GLVertexBatch.h.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CGLVertexBatch g_GLBatch;
//----------------------------------------------------------------------------------
void CGLVertexBatch::Reserve()
{
    // Almost every batch is a four-vertex quad; the circle is the outlier at
    // 362. Reserving once keeps these off the allocator entirely.
    m_Positions.reserve(768);
    m_TexCoords.reserve(768);
    m_Colors.reserve(1536);
    m_Normals.reserve(1152);
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::Begin(GLenum mode, bool textured)
{
    m_Mode = mode;
    m_Textured = textured;
    m_Colored = false;
    m_Normaled = false;

    m_Positions.clear();
    m_TexCoords.clear();
    m_Colors.clear();
    m_Normals.clear();

    m_CurrentTexCoord[0] = 0.0f;
    m_CurrentTexCoord[1] = 0.0f;
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::SeedColorsFromGL()
{
    // Vertices emitted before the first Color() call inherited whatever glColor
    // state was current, so read it back and give them that, rather than letting
    // the new colour apply retroactively to the whole batch.
    GLfloat current[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glGetFloatv(GL_CURRENT_COLOR, current);

    const size_t emitted = m_Positions.size() / 2;
    m_Colors.clear();
    m_Colors.reserve((emitted + 1) * 4);

    for (size_t i = 0; i < emitted; i++)
    {
        m_Colors.push_back(current[0]);
        m_Colors.push_back(current[1]);
        m_Colors.push_back(current[2]);
        m_Colors.push_back(current[3]);
    }
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::SeedNormalsFromGL()
{
    GLfloat current[3] = { 0.0f, 0.0f, 1.0f };
    glGetFloatv(GL_CURRENT_NORMAL, current);

    const size_t emitted = m_Positions.size() / 2;
    m_Normals.clear();
    m_Normals.reserve((emitted + 1) * 3);

    for (size_t i = 0; i < emitted; i++)
    {
        m_Normals.push_back(current[0]);
        m_Normals.push_back(current[1]);
        m_Normals.push_back(current[2]);
    }
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::TexCoord(float u, float v)
{
    m_CurrentTexCoord[0] = u;
    m_CurrentTexCoord[1] = v;
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::Color(float r, float g, float b, float a)
{
    if (!m_Colored)
    {
        m_Colored = true;
        SeedColorsFromGL();
    }

    m_CurrentColor[0] = r;
    m_CurrentColor[1] = g;
    m_CurrentColor[2] = b;
    m_CurrentColor[3] = a;
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::Normal(float x, float y, float z)
{
    if (!m_Normaled)
    {
        m_Normaled = true;
        SeedNormalsFromGL();
    }

    m_CurrentNormal[0] = x;
    m_CurrentNormal[1] = y;
    m_CurrentNormal[2] = z;
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::Vertex(float x, float y)
{
    m_Positions.push_back(x);
    m_Positions.push_back(y);

    if (m_Textured)
    {
        m_TexCoords.push_back(m_CurrentTexCoord[0]);
        m_TexCoords.push_back(m_CurrentTexCoord[1]);
    }

    if (m_Colored)
    {
        m_Colors.push_back(m_CurrentColor[0]);
        m_Colors.push_back(m_CurrentColor[1]);
        m_Colors.push_back(m_CurrentColor[2]);
        m_Colors.push_back(m_CurrentColor[3]);
    }

    if (m_Normaled)
    {
        m_Normals.push_back(m_CurrentNormal[0]);
        m_Normals.push_back(m_CurrentNormal[1]);
        m_Normals.push_back(m_CurrentNormal[2]);
    }
}
//----------------------------------------------------------------------------------
void CGLVertexBatch::End()
{
    const int count = (int)(m_Positions.size() / 2);
    if (count == 0)
        return;

    // Not while one of the client's own shaders is bound. Binding ours over the
    // top of the colorizer or the death shader is what left the world unhued and
    // a ghost looking at it in full colour; those shaders are written against the
    // fixed function arrays below, so let them have them.
    if (UseShaders && g_GLBatchShader.Available() && !g_ClientShaderActive)
    {
        DrawWithShader(count);
        return;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &m_Positions[0]);

    const bool useTexCoords = m_Textured && (int)(m_TexCoords.size() / 2) == count;
    if (useTexCoords)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, &m_TexCoords[0]);
    }

    const bool useColors = m_Colored && (int)(m_Colors.size() / 4) == count;
    if (useColors)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, 0, &m_Colors[0]);
    }

    const bool useNormals = m_Normaled && (int)(m_Normals.size() / 3) == count;
    if (useNormals)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, &m_Normals[0]);
    }

    glDrawArrays(m_Mode, 0, count);

    if (useNormals)
        glDisableClientState(GL_NORMAL_ARRAY);

    if (useColors)
    {
        glDisableClientState(GL_COLOR_ARRAY);
        // A colour array overwrites the current colour on some drivers; put the
        // caller's colour back so the next draw is not tinted by ours.
        glColor4f(
            m_CurrentColor[0], m_CurrentColor[1], m_CurrentColor[2], m_CurrentColor[3]);
    }

    if (useTexCoords)
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glDisableClientState(GL_VERTEX_ARRAY);
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
void CGLVertexBatch::DrawWithShader(int count)
{
    // The shader reads a colour per vertex, so batches that never called Color()
    // take the current fixed function colour - which is what the client sets
    // around most draws, and what those vertices would have been drawn with.
    GLfloat uniformColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (!m_Colored)
        glGetFloatv(GL_CURRENT_COLOR, uniformColor);

    const bool haveTexCoords = m_Textured && (int)(m_TexCoords.size() / 2) == count;
    const bool haveColors = m_Colored && (int)(m_Colors.size() / 4) == count;

    // Lighting is fixed function state, so ask GL whether it is on rather than
    // tracking it separately; the land tile path enables it around its draw.
    const bool lit = (glIsEnabled(GL_LIGHTING) == GL_TRUE) && m_Normaled &&
                     (int)(m_Normals.size() / 3) == count;

    const int floatsPerVertex = 11; // position.xy, texcoord.uv, colour.rgba, normal.xyz
    m_Interleaved.clear();
    m_Interleaved.reserve((size_t)count * floatsPerVertex);

    for (int i = 0; i < count; i++)
    {
        m_Interleaved.push_back(m_Positions[i * 2 + 0]);
        m_Interleaved.push_back(m_Positions[i * 2 + 1]);

        m_Interleaved.push_back(haveTexCoords ? m_TexCoords[i * 2 + 0] : 0.0f);
        m_Interleaved.push_back(haveTexCoords ? m_TexCoords[i * 2 + 1] : 0.0f);

        for (int channel = 0; channel < 4; channel++)
        {
            m_Interleaved.push_back(
                haveColors ? m_Colors[i * 4 + channel] : uniformColor[channel]);
        }

        m_Interleaved.push_back(lit ? m_Normals[i * 3 + 0] : 0.0f);
        m_Interleaved.push_back(lit ? m_Normals[i * 3 + 1] : 0.0f);
        m_Interleaved.push_back(lit ? m_Normals[i * 3 + 2] : 1.0f);
    }

    g_GLBatchShader.SetSourceSize(m_SourceWidth, m_SourceHeight);
    g_GLBatchShader.Draw(
        m_Mode, &m_Interleaved[0], count, floatsPerVertex, haveTexCoords, lit);
}
//----------------------------------------------------------------------------------
