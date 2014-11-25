/* graphene-euler.c: Euler angles
 *
 * Copyright © 2014  Emmanuele Bassi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * SECTION:graphene-euler
 * @Title: Euler
 * @Short_description: Euler angles
 *
 * The #graphene_euler_t structure defines a rotation along three axis using
 * three angles. It also optionally can describe the order of the rotations.
 *
 * Rotations described Euler angles are immediately understandable, compared
 * to rotations expressed using quaternions, but they are susceptible of
 * ["Gimbal lock"](http://en.wikipedia.org/wiki/Gimbal_lock) — the loss of
 * one degree of freedom caused by two axis on the same plane. You typically
 * should use #graphene_euler_t to expose rotation angles in your API, or to
 * store them, but use #graphene_quaternion_t to apply rotations to modelview
 * matrices, or interpolate between initial and final rotation transformations.
 */

#include "graphene-private.h"

#include "graphene-euler.h"

#include "graphene-matrix.h"
#include "graphene-quaternion.h"
#include "graphene-vectors-private.h"

#define EULER_DEFAULT_ORDER     GRAPHENE_EULER_ORDER_XYZ

/**
 * graphene_euler_alloc: (constructor)
 *
 * Allocates a new #graphene_euler_t.
 *
 * The contents of the returned structure are undefined.
 *
 * Returns: (transfer full): the newly allocated #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_alloc (void)
{
  return calloc (1, sizeof (graphene_euler_t));
}

/**
 * graphene_euler_free:
 * @e: a #graphene_euler_t
 *
 * Frees the resources allocated by graphene_euler_alloc().
 *
 * Since: 1.2
 */
void
graphene_euler_free (graphene_euler_t *e)
{
  free (e);
}

/**
 * graphene_euler_init:
 * @e: the #graphene_euler_t to initialize
 * @x: rotation angle on the X axis, in degrees
 * @y: rotation angle on the Y axis, in degrees
 * @z: rotation angle on the Z axis, in degrees
 *
 * Initializes a #graphene_euler_t using the given angles.
 *
 * The order of the rotations is %GRAPHENE_EULER_ORDER_DEFAULT.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init (graphene_euler_t *e,
                     float             x,
                     float             y,
                     float             z)
{
  return graphene_euler_init_with_order (e, x, y, z, GRAPHENE_EULER_ORDER_DEFAULT);
}

/**
 * graphene_euler_init_with_order:
 * @e: the #graphene_euler_t to initialize
 * @x: rotation angle on the X axis
 * @y: rotation angle on the Y axis
 * @z: rotation angle on the Z axis
 * @order: the order used to apply the rotations
 *
 * Initializes a #graphene_euler_t with the given angles and @order.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init_with_order (graphene_euler_t       *e,
                                float                   x,
                                float                   y,
                                float                   z,
                                graphene_euler_order_t  order)
{
  graphene_vec3_init (&e->angles, x, y, z);
  e->order = order;

  return e;
}

/**
 * graphene_euler_init_from_matrix:
 * @e: the #graphene_euler_t to initialize
 * @m: (nullable): a rotation matrix
 * @order: the order used to apply the rotations
 *
 * Initializes a #graphene_euler_t using the given rotation matrix.
 *
 * If the #graphene_matrix_t @m is %NULL, the #graphene_euler_t will
 * be initialized with all angles set to 0.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init_from_matrix (graphene_euler_t        *e,
                                 const graphene_matrix_t *m,
                                 graphene_euler_order_t   order)
{
  float me[16];
  float m11, m12, m13;
  float m21, m22, m23;
  float m31, m32, m33;
  float x, y, z;

  if (m == NULL)
    return graphene_euler_init_with_order (e, 0.f, 0.f, 0.f, order);

  graphene_matrix_to_float (m, me);

  /* isolate the rotation components */
  m11 = me[0]; m21 = me[4]; m31 = me[8];
  m12 = me[1]; m22 = me[5]; m32 = me[9];
  m13 = me[2]; m23 = me[6]; m33 = me[10];

  x = y = z = 0.f;

  e->order = order;
  switch (graphene_euler_get_order (e))
    {
    case GRAPHENE_EULER_ORDER_XYZ:
      y = asinf (CLAMP (m13, -1.f, 1.f));
      if (fabsf (m13) < 1.f)
        {
          x = atan2f (-1.f * m23, m33);
          z = atan2f (-1.f * m12, m11);
        }
      else
        {
          x = atan2f (m32, m22);
          z = 0.f;
        }
      break;

    case GRAPHENE_EULER_ORDER_YXZ:
      x = asinf (-1.f * CLAMP (m23, -1, 1));
      if (fabsf (m23) < 1.f)
        {
          y = atan2f (m13, m33);
          z = atan2f (m21, m22);
        }
      else
        {
          y = atan2f (-1.f * m31, m11);
          z = 0.f;
        }
      break;

    case GRAPHENE_EULER_ORDER_ZXY:
      x = asinf (CLAMP (m32, -1.f, 1.f));

      if (fabsf (m32) < 1.f)
        {
          y = atan2f (-1.f * m31, m33);
          z = atan2f (-1.f * m12, m22);
        }
      else
        {
          y = 0.f;
          z = atan2f (m21, m11);
        }
      break;

    case GRAPHENE_EULER_ORDER_ZYX:
      y = asinf (-1.f * CLAMP (m31, -1.f, 1.f));

      if (fabsf (m31) < 1.f)
        {
          x = atan2f (m32, m33);
          z = atan2f (m21, m11);
        }
      else
        {
          x = 0.f;
          z = atan2f (-1.f * m12, m22);
        }
      break;

    case GRAPHENE_EULER_ORDER_YZX:
      z = asinf (CLAMP (m21, -1.f, 1.f));

      if (fabsf (m21) < 1.f)
        {
          x = atan2f (-1.f * m23, m22);
          y = atan2f (-1.f * m31, m11);
        }
      else
        {
          x = 0.f;
          y = atan2f (m13, m33);
        }
      break;

    case GRAPHENE_EULER_ORDER_XZY:
      z = asinf (-1.f * CLAMP (m12, -1.f, 1.f));

      if (fabsf (m12) < 1.f)
        {
          x = atan2f (m32, m22);
          y = atan2f (m13, m11);
        }
      else
        {
          x = atan2f (-1.f * m23, m33);
          y = 0.f;
        }
      break;

    case GRAPHENE_EULER_ORDER_DEFAULT:
      break;
    }

  graphene_vec3_init (&e->angles,
                      GRAPHENE_RAD_TO_DEG (x),
                      GRAPHENE_RAD_TO_DEG (y),
                      GRAPHENE_RAD_TO_DEG (z));

  return e;
}

/**
 * graphene_euler_init_from_quaternion:
 * @e: a #graphene_euler_t
 * @q: (nullable): a normalized #graphene_quaternion_t
 * @order: the order used to apply the rotations
 *
 * Initializes a #graphene_euler_t using the given normalized quaternion.
 *
 * If the #graphene_quaternion_t @q is %NULL, the #graphene_euler_t will
 * be initialized with all angles set to 0.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init_from_quaternion (graphene_euler_t            *e,
                                     const graphene_quaternion_t *q,
                                     graphene_euler_order_t       order)
{
  float sqx, sqy, sqz, sqw;
  float x, y, z;

  if (q == NULL)
    return graphene_euler_init_with_order (e, 0.f, 0.f, 0.f, order);

  sqx = q->x * q->x;
  sqy = q->y * q->y;
  sqz = q->z * q->z;
  sqw = q->w * q->w;

  x = y = z = 0.f;

  e->order = order;
  switch (graphene_euler_get_order (e))
    {
    case GRAPHENE_EULER_ORDER_XYZ:
      x = atan2f (2.f * (q->x * q->w - q->y * q->z), (sqw - sqx - sqy + sqz));
      y = asinf (CLAMP (2.f * (q->x * q->z + q->y * q->w ), -1.f, 1.f));
      z = atan2f (2.f * (q->z * q->w - q->x * q->y), (sqw + sqx - sqy - sqz));
      break;

    case GRAPHENE_EULER_ORDER_YXZ:
      x = asinf (CLAMP (2.f * (q->x * q->w - q->y * q->z), -1.f, 1.f));
      y = atan2f (2.f * (q->x * q->z + q->y * q->w), (sqw - sqx - sqy + sqz));
      z = atan2f (2.f * (q->x * q->y + q->z * q->w), (sqw - sqx + sqy - sqz));
      break;

    case GRAPHENE_EULER_ORDER_ZXY:
      x = asinf (CLAMP (2.f * (q->x * q->w + q->y * q->z), -1.f, 1.f));
      y = atan2f (2.f * (q->y * q->w - q->z * q->x), (sqw - sqx - sqy + sqz));
      z = atan2f (2.f * (q->z * q->w - q->x * q->y), (sqw - sqx + sqy - sqz));
      break;

    case GRAPHENE_EULER_ORDER_ZYX:
      x = atan2f (2.f * (q->x * q->w + q->z * q->y), (sqw - sqx - sqy + sqz));
      y = asinf (CLAMP (2.f * (q->y * q->w - q->x * q->z), -1.f, 1.f));
      z = atan2f (2.f * (q->x * q->y + q->z * q->w), (sqw + sqx - sqy - sqz));
      break;

    case GRAPHENE_EULER_ORDER_YZX:
      x = atan2f (2.f * (q->x * q->w - q->z * q->y), (sqw - sqx + sqy - sqz));
      y = atan2f (2.f * (q->y * q->w - q->x * q->z), (sqw + sqx - sqy - sqz));
      z = asinf (CLAMP (2.f * (q->x * q->y + q->z * q->w ), -1.f, 1.f));
      break;

    case GRAPHENE_EULER_ORDER_XZY:
      x = atan2f (2.f * (q->x * q->w + q->y * q->z), (sqw - sqx + sqy - sqz));
      y = atan2f (2.f * (q->x * q->z + q->y * q->w), (sqw + sqx - sqy - sqz));
      z = asinf (CLAMP (2.f * (q->z * q->w - q->x * q->y), -1.f, 1.f));
      break;

    case GRAPHENE_EULER_ORDER_DEFAULT:
      break;
    }

  graphene_vec3_init (&e->angles,
                      GRAPHENE_RAD_TO_DEG (x),
                      GRAPHENE_RAD_TO_DEG (y),
                      GRAPHENE_RAD_TO_DEG (z));

  return e;
}

/**
 * graphene_euler_init_from_vec3:
 * @e: the #graphene_euler_t to initialize
 * @v: (nullable): a #graphene_vec3_t
 * @order: the order used to apply the rotations
 *
 * Initializes a #graphene_euler_t using the angles contained in a
 * #graphene_vec3_t.
 *
 * If the #graphene_vec3_t @v is %NULL, the #graphene_euler_t will be
 * initialized with all angles set to 0.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init_from_vec3 (graphene_euler_t       *e,
                               const graphene_vec3_t  *v,
                               graphene_euler_order_t  order)
{
  if (v != NULL)
    graphene_vec3_init_from_vec3 (&e->angles, v);
  else
    graphene_vec3_init_from_vec3 (&e->angles, graphene_vec3_zero ());

  e->order = order;

  return e;
}

/**
 * graphene_euler_init_from_euler:
 * @e: the #graphene_euler_t to initialize
 * @src: (nullable): a #graphene_euler_t
 *
 * Initializes a #graphene_euler_t using the angles and order of
 * another #graphene_euler_t.
 *
 * If the #graphene_euler_t @src is %NULL, this function is equivalent
 * to calling graphene_euler_init() with all angles set to 0.
 *
 * Returns: (transfer none): the initialized #graphene_euler_t
 *
 * Since: 1.2
 */
graphene_euler_t *
graphene_euler_init_from_euler (graphene_euler_t       *e,
                                const graphene_euler_t *src)
{
  if (src == NULL)
    return graphene_euler_init (e, 0.f, 0.f, 0.f);

  *e = *src;

  return e;
}

/**
 * graphene_euler_equal:
 * @a: a #graphene_euler_t
 * @b: a #graphene_euler_t
 *
 * Checks if two #graphene_euler_t are equal.
 *
 * Returns: `true` if the two #graphene_euler_t are equal
 *
 * Since: 1.2
 */
bool
graphene_euler_equal (const graphene_euler_t *a,
                      const graphene_euler_t *b)
{
  if (a == b)
    return true;

  if (a == NULL || b == NULL)
    return false;

  return graphene_vec3_equal (&a->angles, &b->angles) && a->order == b->order;
}

/**
 * graphene_euler_get_x:
 * @e: a #graphene_euler_t
 *
 * Retrieves the rotation angle on the X axis, in degrees.
 *
 * Returns: the rotation angle
 *
 * Since: 1.2
 */
float
graphene_euler_get_x (const graphene_euler_t *e)
{
  return graphene_vec3_get_x (&e->angles);
}

/**
 * graphene_euler_get_y:
 * @e: a #graphene_euler_t
 *
 * Retrieves the rotation angle on the Y axis, in degrees.
 *
 * Returns: the rotation angle
 *
 * Since: 1.2
 */
float
graphene_euler_get_y (const graphene_euler_t *e)
{
  return graphene_vec3_get_y (&e->angles);
}

/**
 * graphene_euler_get_z:
 * @e: a #graphene_euler_t
 *
 * Retrieves the rotation angle on the Z axis, in degrees.
 *
 * Returns: the rotation angle
 *
 * Since: 1.2
 */
float
graphene_euler_get_z (const graphene_euler_t *e)
{
  return graphene_vec3_get_z (&e->angles);
}

/**
 * graphene_euler_get_order:
 * @e: a #graphene_euler_t
 *
 * Retrieves the order used to apply the rotations described in the
 * #graphene_euler_t structure, when converting to and from other
 * structures, like #graphene_quaternion_t and #graphene_matrix_t.
 *
 * Returns: the rotation angle
 *
 * Since: 1.2
 */
graphene_euler_order_t
graphene_euler_get_order (const graphene_euler_t *e)
{
  if (e->order == GRAPHENE_EULER_ORDER_DEFAULT)
    return EULER_DEFAULT_ORDER;

  return e->order;
}

/**
 * graphene_euler_to_vec3:
 * @e: a #graphene_euler_t
 * @res: (out caller-allocates): return location for a #graphene_vec3_t
 *
 * Retrieves the angles of a #graphene_euler_t and initializes a
 * #graphene_vec3_t with them.
 *
 * Since: 1.2
 */
void
graphene_euler_to_vec3 (const graphene_euler_t *e,
                        graphene_vec3_t        *res)
{
  graphene_vec3_init_from_vec3 (res, &e->angles);
}

/**
 * graphene_euler_to_matrix:
 * @e: a #graphene_euler_t
 * @res: (out caller-allocates): return location for a #graphene_matrix_t
 *
 * Converts a #graphene_euler_t into a transformation matrix expressing
 * the rotations described by the Euler angles.
 *
 * Since: 1.2
 */
void
graphene_euler_to_matrix (const graphene_euler_t *e,
                          graphene_matrix_t      *res)
{
  graphene_euler_order_t order = graphene_euler_get_order (e);
  float x = GRAPHENE_DEG_TO_RAD (graphene_vec3_get_x (&e->angles));
  float y = GRAPHENE_DEG_TO_RAD (graphene_vec3_get_y (&e->angles));
  float z = GRAPHENE_DEG_TO_RAD (graphene_vec3_get_z (&e->angles));
  float cos_x = cosf (x), sin_x = sinf (x);
  float cos_y = cosf (y), sin_y = sinf (y);
  float cos_z = cosf (z), sin_z = sinf (z);
  graphene_vec4_t row_x, row_y, row_z, row_w;

  switch (order)
    {
    case GRAPHENE_EULER_ORDER_XYZ:
      {
        float ae = cos_x * cos_z, af = cos_x * sin_z, be = sin_x * cos_z, bf = sin_x * sin_z;

        graphene_vec4_init (&row_x, cos_y * cos_z, -1.f * cos_y * sin_z, sin_y, 0.f);
        graphene_vec4_init (&row_y, af + be * sin_y, ae - bf * sin_y, -1.f * sin_x * cos_y, 0.f);
        graphene_vec4_init (&row_z, bf - ae * sin_y, be + af * sin_y, cos_x * cos_y, 0.f);
      }
      break;

    case GRAPHENE_EULER_ORDER_YXZ:
      {
        float ce = cos_y * cos_z, cf = cos_y * sin_z, de = sin_y * cos_z, df = sin_y * sin_z;

        graphene_vec4_init (&row_x, ce + df * sin_x, de * sin_x - cf, cos_x * sin_y, 0.f);
        graphene_vec4_init (&row_y, cos_x * sin_z, cos_x * cos_z, -1.f * sin_x, 0.f);
        graphene_vec4_init (&row_z, cf * sin_x - de, df + ce * sin_x, cos_x * cos_y, 0.f);
      }
      break;

    case GRAPHENE_EULER_ORDER_ZXY:
      {
        float ce = cos_y * cos_z, cf = cos_y * sin_z, de = sin_y * cos_z, df = sin_y * sin_z;

        graphene_vec4_init (&row_x, ce - df * sin_x, -1.f * cos_x * sin_z, de + cf * sin_x, 0.f);
        graphene_vec4_init (&row_y, cf + de * sin_x, cos_x * cos_z, df - ce * sin_x, 0.f);
        graphene_vec4_init (&row_z, -1.f * cos_x * sin_y, sin_x, cos_x * cos_y, 0.f);
      }
      break;

    case GRAPHENE_EULER_ORDER_ZYX:
      {
        float ae = cos_x * cos_z, af = cos_x * sin_z, be = sin_x * cos_z, bf = sin_x * sin_z;

        graphene_vec4_init (&row_x, cos_y * cos_z, be * sin_y - af, ae * sin_y + bf, 0.f);
        graphene_vec4_init (&row_y, cos_y * sin_z, bf * sin_y + ae, af * sin_y - be, 0.f);
        graphene_vec4_init (&row_z, -1.f * sin_y, sin_x * cos_y, cos_x * cos_y, 0.f);
      }
      break;

    case GRAPHENE_EULER_ORDER_YZX:
      {
        float ac = cos_x * cos_y, ad = cos_x * sin_y, bc = sin_x * cos_y, bd = sin_x * sin_y;

        graphene_vec4_init (&row_x, cos_y * cos_z, bd - ac * sin_z, bc * sin_z + ad, 0.f);
        graphene_vec4_init (&row_y, sin_z, cos_x * cos_z, -1.f * sin_x * cos_z, 0.f);
        graphene_vec4_init (&row_z, -1.f * sin_y * cos_z, ad * sin_z + bc, ac - bd * sin_z, 0.f);
      }
      break;

    case GRAPHENE_EULER_ORDER_XZY:
      {
        float ac = cos_x * cos_y, ad = cos_x * sin_y, bc = sin_x * cos_y, bd = sin_x * sin_y;

        graphene_vec4_init (&row_x, cos_y * cos_z, -1.f * sin_z, sin_y * cos_z, 0.f);
        graphene_vec4_init (&row_y, ac * sin_z + bd, cos_x * cos_z, ad * sin_z - bc, 0.f);
        graphene_vec4_init (&row_z, bc * sin_z - ad, sin_x * cos_z, bd * sin_z + ac, 0.f);
      }
      break;

    default:
      graphene_vec4_init (&row_x, 1.f, 0.f, 0.f, 0.f);
      graphene_vec4_init (&row_y, 0.f, 1.f, 0.f, 0.f);
      graphene_vec4_init (&row_z, 0.f, 0.f, 1.f, 0.f);
      break;
    }

  graphene_vec4_init (&row_z, 0.f, 0.f, 0.f, 1.f);

  graphene_matrix_init_from_vec4 (res, &row_x, &row_y, &row_z, &row_w);
}

/**
 * graphene_euler_reorder:
 * @e: a #graphene_euler_t
 * @res: (out caller-allocates): return location for the reordered
 *   #graphene_euler_t
 *
 * Reorders a #graphene_euler_t.
 *
 * This function is equivalent to creating a #graphene_quaternion_t from the
 * given #graphene_euler_t, and then converting the quaternion into another
 * #graphene_euler_t.
 *
 * This function does not preserve the order of @e when reordering, and
 * sets the order of @res to %GRAPHENE_EULER_ORDER_DEFAULT.
 *
 * Since: 1.2
 */
void
graphene_euler_reorder (const graphene_euler_t *e,
                        graphene_euler_t       *res)
{
  graphene_quaternion_t q;

  graphene_quaternion_init_from_euler (&q, e);
  graphene_euler_init_from_quaternion (res, &q, GRAPHENE_EULER_ORDER_DEFAULT);
}